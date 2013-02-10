package com.sos.scheduler.engine.kernel;

import com.google.inject.Injector;
import com.google.inject.Module;
import com.sos.scheduler.engine.common.xml.NamedChildElements;
import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyInvalidatedException;
import com.sos.scheduler.engine.cplusplus.runtime.DisposableCppProxyRegister;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.data.log.SchedulerLogLevel;
import com.sos.scheduler.engine.data.scheduler.SchedulerCloseEvent;
import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import com.sos.scheduler.engine.kernel.command.CommandSubsystem;
import com.sos.scheduler.engine.kernel.command.UnknownCommandException;
import com.sos.scheduler.engine.kernel.configuration.SchedulerModule;
import com.sos.scheduler.engine.kernel.cppproxy.HttpResponseC;
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC;
import com.sos.scheduler.engine.kernel.event.EventSubsystem;
import com.sos.scheduler.engine.kernel.event.OperationExecutor;
import com.sos.scheduler.engine.kernel.http.SchedulerHttpRequest;
import com.sos.scheduler.engine.kernel.http.SchedulerHttpResponse;
import com.sos.scheduler.engine.kernel.job.JobSubsystem;
import com.sos.scheduler.engine.kernel.log.CppLogger;
import com.sos.scheduler.engine.kernel.log.LogCategory;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.order.OrderSubsystem;
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystem;
import com.sos.scheduler.engine.kernel.scheduler.*;
import com.sos.scheduler.engine.kernel.time.TimeZones;
import com.sos.scheduler.engine.kernel.variable.VariableSet;
import com.sos.scheduler.engine.main.SchedulerControllerBridge;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

import javax.annotation.Nullable;
import java.util.TimeZone;

import static com.google.common.base.Objects.firstNonNull;
import static com.google.common.base.Preconditions.checkArgument;
import static com.google.inject.Guice.createInjector;
import static com.sos.scheduler.engine.common.log.LoggingFunctions.enableJavaUtilLoggingOverSLF4J;
import static com.sos.scheduler.engine.common.xml.XmlUtils.childElements;
import static com.sos.scheduler.engine.common.xml.XmlUtils.loadXml;
import static org.joda.time.DateTimeZone.UTC;

@ForCpp
public final class Scheduler
implements Sister, SchedulerIsClosed, SchedulerXmlCommandExecutor, SchedulerHttpService, HasGuiceModule, HasInjector {
    private static final Logger logger = LoggerFactory.getLogger(Scheduler.class);

    private final SpoolerC cppProxy;
    private final SchedulerControllerBridge controllerBridge;
    private boolean threadInitiallyLocked = false;
    private boolean closed = false;
    private final Module guiceModule;
    private final Injector injector;
    private final PrefixLog _log;
    private final DisposableCppProxyRegister disposableCppProxyRegister;
    private final SchedulerEventBus eventBus;
    private final PluginSubsystem pluginSubsystem;
    private final CommandSubsystem commandSubsystem;
    private final OperationExecutor operationExecutor;

    @ForCpp public Scheduler(SpoolerC cppProxy, @Nullable SchedulerControllerBridge controllerBridgeOrNull) {
        staticInitialize();
        this.cppProxy = cppProxy;
        this.cppProxy.setSister(this);
        this.controllerBridge = firstNonNull(controllerBridgeOrNull, EmptySchedulerControllerBridge.singleton);
        controllerBridge.getSettings().setSettingsInCpp(cppProxy.modifiable_settings());
        guiceModule = new SchedulerModule(cppProxy, controllerBridge, this);
        injector = createInjector(guiceModule);
        _log = injector.getInstance(PrefixLog.class);
        disposableCppProxyRegister = injector.getInstance(DisposableCppProxyRegister.class);
        eventBus = injector.getInstance(SchedulerEventBus.class);
        pluginSubsystem = injector.getInstance(PluginSubsystem.class);
        commandSubsystem = injector.getInstance(CommandSubsystem.class);
        operationExecutor = injector.getInstance(OperationExecutor.class);
        initialize();
    }

    private static void staticInitialize() {
        enableJavaUtilLoggingOverSLF4J();
        TimeZones.initialize();
        //DateTimeZone.setDefault(UTC);
        TimeZone.setDefault(UTC.toTimeZone());       // Für JPA @Temporal(TIMESTAMP), damit Date wirklich UTC enthält. Siehe http://stackoverflow.com/questions/508019
    }

    private void initialize() {
        Thread.currentThread().setContextClassLoader(getClass().getClassLoader());   // http://stackoverflow.com/questions/1969667
        initializeThreadLock();
    }

    private void initializeThreadLock() {
        if (!isStartedByJava()) { // Wenn wir ein controllerBridge haben, ist der Scheduler über Java (CppScheduler.main) aufgerufen worden. Dort wird die Sperre gesetzt.
            threadLock();
            threadInitiallyLocked = true;
        }
    }

    public Injector getInjector() {
        return injector;
    }

    @Override public Module getGuiceModule() {
        return guiceModule;
    }

    @Override public void onCppProxyInvalidated() {}

    @ForCpp public void onClose() {
        closed = true;
        try {
            eventBus.publish(new SchedulerCloseEvent());
            eventBus.dispatchEvents();
            try {
                pluginSubsystem.close();
            } catch (Exception x) {
                log().error("pluginSubsystem.close(): " + x);
            }
        } finally {
            if (threadInitiallyLocked) {
                threadUnlock();     //TODO Sperre wird zu früh freigegeben, der Scheduler läuft ja noch. Lösung: Start über Java mit CppScheduler.run()
                threadInitiallyLocked = false;
            }
        }
        eventBus.dispatchEvents();
        disposableCppProxyRegister.tryDisposeAll();
    }

    @Override public boolean isClosed() {
        return closed;
    }

    @ForCpp public void onLoad(String configurationXml) {
        onLoad(loadXml(configurationXml).getDocumentElement());
    }

    private void onLoad(Element configElement) {
        pluginSubsystem.load(configElement);
        controllerBridge.onSchedulerStarted(this);
    }

    @ForCpp public void onActivate() {
        pluginSubsystem.activate();
    }

    @ForCpp public void onActivated() {
        controllerBridge.onSchedulerActivated();
    }

    @ForCpp public void onEnteringSleepState() {    //TODO Name ändern in onSchedulerStep(), wird bei jedem Schleifendurchlauf aufgerufen.
        eventBus.dispatchEvents();
        operationExecutor.execute();
    }

    public void terminate() {
        try {
            cppProxy.cmd_terminate();
        } catch (CppProxyInvalidatedException x) {
            logger.debug("Scheduler.terminate() ignored because C++ object has already been destroyed", x);
        }
    }

    private boolean isStartedByJava() {
        return controllerBridge != EmptySchedulerControllerBridge.singleton;
    }

    @ForCpp public void threadLock() {
        CppProxy.threadLock.lock();
    }

    @ForCpp public void threadUnlock() {
        CppProxy.threadLock.unlock();
    }

    /** @return {@link HttpResponseC#close()} MUSS aufgerufen werden! */
    @Override public HttpResponseC executeHttpRequest(SchedulerHttpRequest request, SchedulerHttpResponse response) {
        return cppProxy.java_execute_http(request, response);
    }

    /** Stellt XML-Prolog voran und löst bei einem ERROR-Element eine Exception aus. */
    @Override public String executeXml(String xml) {
        checkArgument(!xml.startsWith("<?"), "executeXml() does not accept XML with a prolog");  // Blanks und Kommentare vereiteln diese Prüfung.
        String prolog = "<?xml version='1.0' encoding='iso-8859-1'?>";   // Für libxml2, damit Umlaute korrekt erkant werden.
        String result = uncheckedExecuteXml(prolog + xml);
        if (result.contains("<ERROR")) {
            Document doc = loadXml(result);
            for (Element e: childElements(doc.getDocumentElement()))
                for (Element error: new NamedChildElements("ERROR", e))
                    throw new SchedulerException(error.getAttribute("code") +" "+ error.getAttribute("text"));
        }
        return result;
    }

    /** execute_xml() der C++-Klasse Spooler */
    @Override public String uncheckedExecuteXml(String xml) {
        return cppProxy.execute_xml(xml);
    }

    /** Nur für C++, zur Ausführung eines Kommandos in Java */
    @ForCpp public String javaExecuteXml(String xml) {
        try {
            return commandSubsystem.executeXml(xml);
        } catch (UnknownCommandException x) {
            _log.warn(x.toString());
            return "UNKNOWN_COMMAND";   // Siehe command_error.cxx, für ordentliche Meldung SCHEDULER-105, bis Java die selbst liefert kann.
        }
    }

    public SchedulerConfiguration getConfiguration() {
        return injector.getInstance(SchedulerConfiguration.class);
    }

    @ForCpp public EventSubsystem getEventSubsystem() {
        return injector.getInstance(EventSubsystem.class);
    }

    public JobSubsystem getJobSubsystem() {
        return injector.getInstance(JobSubsystem.class);
    }

    public OrderSubsystem getOrderSubsystem() {
        return injector.getInstance(OrderSubsystem.class);
    }

    public String getHttpUrl() {
        return cppProxy.http_url();
    }

    public int getTcpPort() {
        return cppProxy.tcp_port();
    }

    /** Das ist einfach der C-Systemaufruf gethostname(). */
    @Deprecated
    public String getHostname() {
        return cppProxy.hostname();
    }

    public void callCppAndDoNothing() {
        cppProxy.tcp_port();
    }

    @ForCpp public void log(String prefix, int level, String line) {
        CppLogger.log(prefix, SchedulerLogLevel.ofCpp(level), line);
    }

    /** @param text Sollte auf \n enden */
    public void writeToSchedulerLog(LogCategory category, String text) {
        cppProxy.write_to_scheduler_log(category.asString(), text);
    }

    public VariableSet getVariables() {
        return cppProxy.variables().getSister();
    }

    public PrefixLog log() {
        return _log;
    }
}
