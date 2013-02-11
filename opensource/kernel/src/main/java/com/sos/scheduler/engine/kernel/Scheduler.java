package com.sos.scheduler.engine.kernel;

import com.google.inject.Injector;
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
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC;
import com.sos.scheduler.engine.kernel.event.EventSubsystem;
import com.sos.scheduler.engine.kernel.event.OperationExecutor;
import com.sos.scheduler.engine.kernel.log.CppLogger;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystem;
import com.sos.scheduler.engine.kernel.scheduler.*;
import com.sos.scheduler.engine.kernel.time.TimeZones;
import com.sos.scheduler.engine.main.SchedulerControllerBridge;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

import javax.annotation.Nullable;
import javax.inject.Inject;
import javax.inject.Singleton;
import java.util.TimeZone;

import static com.google.common.base.Objects.firstNonNull;
import static com.google.inject.Guice.createInjector;
import static com.sos.scheduler.engine.common.log.LoggingFunctions.enableJavaUtilLoggingOverSLF4J;
import static com.sos.scheduler.engine.common.xml.XmlUtils.childElements;
import static com.sos.scheduler.engine.common.xml.XmlUtils.loadXml;
import static org.joda.time.DateTimeZone.UTC;

@ForCpp
@Singleton
public final class Scheduler
implements Sister, SchedulerIsClosed, SchedulerXmlCommandExecutor, HasInjector {
    private static final Logger logger = LoggerFactory.getLogger(Scheduler.class);

    private final SpoolerC cppProxy;
    private final SchedulerControllerBridge controllerBridge;
    private final PrefixLog prefixLog;
    private final DisposableCppProxyRegister disposableCppProxyRegister;
    private final PluginSubsystem pluginSubsystem;
    private final CommandSubsystem commandSubsystem;
    private final OperationExecutor operationExecutor;
    private final SchedulerEventBus eventBus;
    private final Injector injector;
    private boolean threadInitiallyLocked = false;
    private boolean closed = false;

    @Inject private Scheduler(SpoolerC cppProxy, SchedulerControllerBridge controllerBridge, PrefixLog prefixLog,
            DisposableCppProxyRegister disposableCppProxyRegister,
            PluginSubsystem pluginSubsystem, CommandSubsystem commandSubsystem, OperationExecutor operationExecutor,
            SchedulerEventBus eventBus, Injector injector) {
        staticInitialize();
        this.cppProxy = cppProxy;
        this.cppProxy.setSister(this);
        this.controllerBridge = controllerBridge;
        this.prefixLog = prefixLog;
        this.disposableCppProxyRegister = disposableCppProxyRegister;
        this.pluginSubsystem = pluginSubsystem;
        this.commandSubsystem = commandSubsystem;
        this.operationExecutor = operationExecutor;
        this.eventBus = eventBus;
        this.injector = injector;
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

    @Override public void onCppProxyInvalidated() {}

    @ForCpp private void onClose() {
        closed = true;
        try {
            eventBus.publish(new SchedulerCloseEvent());
            eventBus.dispatchEvents();
            try {
                pluginSubsystem.close();
            } catch (Exception x) {
                prefixLog.error("pluginSubsystem.close(): " + x);
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

    @ForCpp private void onLoad(String configurationXml) {
        pluginSubsystem.load(loadXml(configurationXml).getDocumentElement());
        controllerBridge.onSchedulerStarted(this);
    }

    @ForCpp private void onActivate() {
        pluginSubsystem.activate();
    }

    @ForCpp private void onActivated() {
        controllerBridge.onSchedulerActivated();
    }

    @ForCpp private void onEnteringSleepState() {    //TODO Name ändern in onSchedulerStep(), wird bei jedem Schleifendurchlauf aufgerufen.
        eventBus.dispatchEvents();
        operationExecutor.execute();
    }

    /** Nur für C++, zur Ausführung eines Kommandos in Java */
    @ForCpp private String javaExecuteXml(String xml) {
        try {
            return commandSubsystem.executeXml(xml);
        } catch (UnknownCommandException x) {
            prefixLog.warn(x.toString());
            return "UNKNOWN_COMMAND";   // Siehe command_error.cxx, für ordentliche Meldung SCHEDULER-105, bis Java die selbst liefert kann.
        }
    }

    @ForCpp private EventSubsystem getEventSubsystem() {
        return injector.getInstance(EventSubsystem.class);
    }

    @ForCpp private void log(String prefix, int level, String line) {
        CppLogger.log(prefix, SchedulerLogLevel.ofCpp(level), line);
    }

    @ForCpp private void threadLock() {
        CppProxy.threadLock.lock();
    }

    @ForCpp private void threadUnlock() {
        CppProxy.threadLock.unlock();
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

    /** Stellt XML-Prolog voran und löst bei einem ERROR-Element eine Exception aus. */
    @Override public String executeXml(String xml) {
        String result = uncheckedExecuteXml(xml);
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

    @Override public Injector getInjector() {
        return injector;
    }

    @Override public boolean isClosed() {
        return closed;
    }

    public void callCppAndDoNothing() {
        cppProxy.tcp_port();
    }

//    /** @param text Sollte auf \n enden */
//    public void writeToSchedulerLog(LogCategory category, String text) {
//        cppProxy.write_to_scheduler_log(category.asString(), text);
//    }

    @ForCpp private static Scheduler of(SpoolerC cppProxy, @Nullable SchedulerControllerBridge controllerBridgeOrNull) {
        SchedulerControllerBridge controllerBridge = firstNonNull(controllerBridgeOrNull, EmptySchedulerControllerBridge.singleton);
        controllerBridge.getSettings().setSettingsInCpp(cppProxy.modifiable_settings());
        Injector injector = createInjector(new SchedulerModule(cppProxy, controllerBridge));
        return injector.getInstance(Scheduler.class);
    }
}