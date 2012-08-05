package com.sos.scheduler.engine.kernel;

import com.google.common.collect.ImmutableList;
import com.google.inject.*;
import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyInvalidatedException;
import com.sos.scheduler.engine.cplusplus.runtime.DisposableCppProxyRegister;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.data.scheduler.SchedulerCloseEvent;
import com.sos.scheduler.engine.eventbus.EventBus;
import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import com.sos.scheduler.engine.kernel.command.CommandHandler;
import com.sos.scheduler.engine.kernel.command.CommandSubsystem;
import com.sos.scheduler.engine.kernel.command.HasCommandHandlers;
import com.sos.scheduler.engine.kernel.command.UnknownCommandException;
import com.sos.scheduler.engine.kernel.cppproxy.HttpResponseC;
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC;
import com.sos.scheduler.engine.kernel.database.DatabaseSubsystem;
import com.sos.scheduler.engine.kernel.event.EventSubsystem;
import com.sos.scheduler.engine.kernel.event.OperationExecutor;
import com.sos.scheduler.engine.kernel.event.OperationQueue;
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem;
import com.sos.scheduler.engine.kernel.http.SchedulerHttpRequest;
import com.sos.scheduler.engine.kernel.http.SchedulerHttpResponse;
import com.sos.scheduler.engine.kernel.job.JobSubsystem;
import com.sos.scheduler.engine.kernel.log.LogCategory;
import com.sos.scheduler.engine.kernel.log.LogSubsystem;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.log.SchedulerLog;
import com.sos.scheduler.engine.kernel.order.OrderSubsystem;
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystem;
import com.sos.scheduler.engine.kernel.scheduler.*;
import com.sos.scheduler.engine.kernel.util.Lazy;
import com.sos.scheduler.engine.kernel.variable.VariableSet;
import com.sos.scheduler.engine.main.SchedulerControllerBridge;
import com.sos.scheduler.engine.util.xml.NamedChildElements;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

import javax.annotation.Nullable;
import javax.persistence.EntityManager;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

import static com.google.common.base.Objects.firstNonNull;
import static com.google.common.base.Preconditions.checkArgument;
import static com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.version;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.childElements;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.loadXml;
import static com.sos.scheduler.engine.util.LoggingFunctions.enableJavaUtilLoggingOverSLF4J;

@ForCpp
public final class Scheduler implements Sister,
        SchedulerIsClosed, SchedulerXmlCommandExecutor, SchedulerHttpService, HasGuiceModule {
    private static final Logger logger = LoggerFactory.getLogger(Scheduler.class);

    private final SchedulerInstanceId instanceId = new SchedulerInstanceId(UUID.randomUUID().toString());
    private final SpoolerC cppProxy;
    private final SchedulerConfiguration configuration;
    private final SchedulerControllerBridge controllerBridge;
    private final PrefixLog _log;   // Unterstrich dem Scala-IntelliJ-Plugin zuliebe. Zschimmer 10.12.2011
    private final SchedulerEventBus eventBus;
    private final OperationExecutor operationExecutor;

    private final LogSubsystem logSubsystem;
    private final DatabaseSubsystem databaseSubsystem;
    private final PluginSubsystem pluginSubsystem;
    private final FolderSubsystem folderSubsystem;
    private final JobSubsystem jobSubsystem;
    private final OrderSubsystem orderSubsystem;
    private final EventSubsystem eventSubsystem;
    private final CommandSubsystem commandSubsystem;
    private boolean threadInitiallyLocked = false;
    private final DisposableCppProxyRegister disposableCppProxyRegister = new DisposableCppProxyRegister();
    private boolean closed = false;
    private final Lazy<Injector> injectorLazy = new Lazy<Injector>() {
        @Override protected Injector compute() { return Guice.createInjector(guiceModule.get()); }
    };

    private final Lazy<Module> guiceModule = new Lazy<Module>() {
        @Override protected Module compute() {
            return new AbstractModule() {
                @Override protected void configure() {
                    bind(EventBus.class).toInstance(eventBus);
                    bind(DatabaseSubsystem.class).toInstance(databaseSubsystem);
                    bind(DisposableCppProxyRegister.class).toInstance(disposableCppProxyRegister);
                    bind(EntityManager.class).toProvider(new Provider<EntityManager>(){
                        @Override public EntityManager get() {
                            return databaseSubsystem.getEntityManager();
                        }
                    });
                    bind(FolderSubsystem.class).toInstance(folderSubsystem);
                    bind(HasGuiceModule.class).toInstance(Scheduler.this);  // Für JettyPlugin
                    bind(JobSubsystem.class).toInstance(jobSubsystem);
                    bind(OrderSubsystem.class).toInstance(orderSubsystem);
                    bind(OperationQueue.class).toInstance(operationExecutor);
                    bind(PluginSubsystem.class).toInstance(pluginSubsystem);
                    bind(PrefixLog.class).toInstance(_log);
                    bind(Scheduler.class).toInstance(Scheduler.this);
                    bind(SchedulerConfiguration.class).toInstance(configuration);
                    bind(SchedulerHttpService.class).toInstance(Scheduler.this);
                    bind(SchedulerInstanceId.class).toInstance(instanceId);
                    bind(SchedulerIsClosed.class).toInstance(Scheduler.this);
                    bind(SchedulerXmlCommandExecutor.class).toInstance(Scheduler.this);
                }
            };
        }
    };

    @ForCpp public Scheduler(SpoolerC cppProxy, @Nullable SchedulerControllerBridge controllerBridgeOrNull) {
        this.cppProxy = cppProxy;
        configuration = new SchedulerConfiguration(cppProxy);
        controllerBridge = firstNonNull(controllerBridgeOrNull, EmptySchedulerControllerBridge.singleton);
        cppProxy.setSister(this);
        controllerBridge.getSettings().setSettingsInCpp(cppProxy.modifiable_settings());

        _log = cppProxy.log().getSister();
        eventBus = controllerBridge.getEventBus();
        operationExecutor = new OperationExecutor(_log);

        logSubsystem = new LogSubsystem(new SchedulerLog(this.cppProxy));
        eventSubsystem = new EventSubsystem(eventBus);
        databaseSubsystem = new DatabaseSubsystem(this.cppProxy.db());
        folderSubsystem = new FolderSubsystem(this.cppProxy.folder_subsystem());
        jobSubsystem = new JobSubsystem(this.cppProxy.job_subsystem());
        orderSubsystem = new OrderSubsystem(this.cppProxy.order_subsystem());
        pluginSubsystem = new PluginSubsystem(this, injectorLazy, eventBus);
        commandSubsystem = new CommandSubsystem(getCommandHandlers(ImmutableList.of(pluginSubsystem)));

        enableJavaUtilLoggingOverSLF4J();
        initializeThreadLock();
    }

    public Injector getInjector() {
        return injectorLazy.get();
    }

    @Override public Module getGuiceModule() {
        return guiceModule.get();
    }

    private static Iterable<CommandHandler> getCommandHandlers(Iterable<?> objects) {
        List<CommandHandler> result = new ArrayList<CommandHandler>();
        for (Object o : objects)
            if (o instanceof HasCommandHandlers)
                result.addAll(((HasCommandHandlers)o).getCommandHandlers());
        return result;
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
            try {
                logSubsystem.close();
            } catch (Exception x) {
                log().error("logSubsystem.close(): " + x);
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
        logSubsystem.activate();
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

    private void initializeThreadLock() {
        if (!isStartedByJava()) { // Wenn wir ein controllerBridge haben, ist der Scheduler über Java (CppScheduler.main) aufgerufen worden. Dort wird die Sperre gesetzt.
            threadLock();
            threadInitiallyLocked = true;
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
        return configuration;
    }

    @ForCpp public EventSubsystem getEventSubsystem() {
        return eventSubsystem;
    }

    public JobSubsystem getJobSubsystem() {
        return jobSubsystem;
    }

    public OrderSubsystem getOrderSubsystem() {
        return orderSubsystem;
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
