package com.sos.scheduler.engine.kernel;

import static com.google.common.base.Objects.firstNonNull;
import static com.google.common.base.Preconditions.checkArgument;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.loadXml;

import java.util.ArrayList;
import java.util.List;

import javax.annotation.Nullable;

import org.apache.log4j.Logger;
import org.w3c.dom.Element;

import com.google.common.collect.ImmutableList;
import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.eventbus.EventBus;
import com.sos.scheduler.engine.eventbus.SchedulerEventBus;
import com.sos.scheduler.engine.kernel.command.CommandHandler;
import com.sos.scheduler.engine.kernel.command.CommandSubsystem;
import com.sos.scheduler.engine.kernel.command.HasCommandHandlers;
import com.sos.scheduler.engine.kernel.command.UnknownCommandException;
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC;
import com.sos.scheduler.engine.kernel.database.DatabaseSubsystem;
import com.sos.scheduler.engine.kernel.event.EventSubsystem;
import com.sos.scheduler.engine.kernel.event.OperationExecutor;
import com.sos.scheduler.engine.kernel.event.OperationQueue;
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem;
import com.sos.scheduler.engine.kernel.job.JobSubsystem;
import com.sos.scheduler.engine.kernel.log.LogCategory;
import com.sos.scheduler.engine.kernel.log.LogSubsystem;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.log.SchedulerLog;
import com.sos.scheduler.engine.kernel.order.OrderSubsystem;
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystem;
import com.sos.scheduler.engine.kernel.scheduler.EmptySchedulerControllerBridge;
import com.sos.scheduler.engine.kernel.scheduler.HasPlatform;
import com.sos.scheduler.engine.kernel.scheduler.Platform;
import com.sos.scheduler.engine.kernel.scheduler.events.SchedulerCloseEvent;
import com.sos.scheduler.engine.kernel.util.Lazy;
import com.sos.scheduler.engine.kernel.util.MavenProperties;
import com.sos.scheduler.engine.kernel.variable.VariableSet;
import com.sos.scheduler.engine.main.SchedulerControllerBridge;

@ForCpp
public final class Scheduler implements HasPlatform, Sister {
    private static final Logger logger = Logger.getLogger(Scheduler.class);

    private final MavenProperties mavenProperties = new MavenProperties(Scheduler.class);
    private final SpoolerC cppProxy;
    private final SchedulerControllerBridge controllerBridge;
    private final PrefixLog log_;   // Unterstrich dem Scala-IntelliJ-Plugin zuliebe. Zschimmer 10.12.2011
    private final Platform platform;
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

    @ForCpp public Scheduler(SpoolerC cppProxy, @Nullable SchedulerControllerBridge controllerBridgeOrNull) {
        this.cppProxy = cppProxy;
        controllerBridge = firstNonNull(controllerBridgeOrNull, EmptySchedulerControllerBridge.singleton);
        cppProxy.setSister(this);
        controllerBridge.getSettings().setSettingsInCpp(cppProxy.modifiable_settings());

        log_ = new PrefixLog(cppProxy.log());
        platform = new Platform(log_);
        eventBus = controllerBridge.getEventBus();
        operationExecutor = new OperationExecutor(log_);

        logSubsystem = new LogSubsystem(new SchedulerLog(this.cppProxy));
        eventSubsystem = new EventSubsystem(platform, eventBus);
        databaseSubsystem = new DatabaseSubsystem(this.cppProxy.db());
        folderSubsystem = new FolderSubsystem(this.cppProxy.folder_subsystem());
        jobSubsystem = new JobSubsystem(platform, this.cppProxy.job_subsystem());
        orderSubsystem = new OrderSubsystem(platform, this.cppProxy.order_subsystem());
        Lazy<Injector> injector = new Lazy<Injector>() { @Override protected Injector compute() { return newInjector(); } };
        pluginSubsystem = new PluginSubsystem(this, injector, eventBus);
        commandSubsystem = new CommandSubsystem(getCommandHandlers(ImmutableList.of(pluginSubsystem)));

        initializeThreadLock();
    }

    private Injector newInjector() {
        return Guice.createInjector(new AbstractModule() {
            @Override protected void configure() {
                bind(DatabaseSubsystem.class).toInstance(databaseSubsystem);
                bind(FolderSubsystem.class).toInstance(folderSubsystem);
                bind(JobSubsystem.class).toInstance(jobSubsystem);
                bind(OrderSubsystem.class).toInstance(orderSubsystem);
                bind(OperationQueue.class).toInstance(operationExecutor);
                bind(EventBus.class).toInstance(eventBus);
                bind(PrefixLog.class).toInstance(log_);
            }
        });
    }

    private static Iterable<CommandHandler> getCommandHandlers(Iterable<?> objects) {
        List<CommandHandler> result = new ArrayList<CommandHandler>();
        for (Object o : objects)
            if (o instanceof HasCommandHandlers)
                result.addAll(((HasCommandHandlers)o).getCommandHandlers());
        return result;
    }

    @Override public Platform getPlatform() {
        return platform;
    }

    @Override public PrefixLog log() {
        return log_;
    }

    @Override public void onCppProxyInvalidated() {}

    @ForCpp public void onClose() {
        try {
            eventBus.publish(new SchedulerCloseEvent());
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
    }

    @ForCpp public void onLoad(String configurationXml) {
        onLoad(loadXml(configurationXml).getDocumentElement());
    }

    private void onLoad(Element configElement) {
        pluginSubsystem.load(configElement);
        controllerBridge.onSchedulerStarted(this);
    }

    @ForCpp public void onActivate() {
        logSubsystem.activate();
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
        if (!cppProxy.cppReferenceIsValid())
            log_.debug("Scheduler.terminate() ignored because C++ object has already been destructed");
        else
            cppProxy.cmd_terminate();
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

    public String executeXml(String xml) {
        checkArgument(!xml.startsWith("<?"), "executeXml() does not accept XML with a prolog");  // Blanks und Kommentare vereiteln diese Prüfung.
        String prolog = "<?xml version='1.0' encoding='iso-8859-1'?>";   // Für libxml2, damit Umlaute korrekt erkant werden.
        return cppProxy.execute_xml(prolog + xml);
    }

    /** Nur für C++, zur Ausführung eines Kommandos in Java */
    @ForCpp public String javaExecuteXml(String xml) {
        try {
            return commandSubsystem.executeXml(xml);
        } catch (UnknownCommandException x) {
            log_.warn(x.toString());
            return "UNKNOWN_COMMAND";   // Siehe command_error.cxx, für ordentliche Meldung SCHEDULER-105, bis Java die selbst liefert kann.
        }
    }

    public FolderSubsystem getFolderSubsystem() {
        return folderSubsystem;
    }

    public DatabaseSubsystem getDatabaseSubsystem() {
        return databaseSubsystem;
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

    public OperationQueue getOperationQueue() {
        return operationExecutor;
    }

    public String getSchedulerId() {
        return cppProxy.id();
    }

    public String getClusterMemberId() {
        return cppProxy.cluster_member_id();
    }

    public String getHttpUrl() {
        return cppProxy.http_url();
    }

    public int getTcpPort() {
        return cppProxy.tcp_port();
    }

    public String getHostname() {
        return cppProxy.hostname();
    }

    public String getHostnameLong() {
        return cppProxy.hostname_complete();
    }

    public void callCppAndDoNothing() {
        cppProxy.tcp_port();
    }

    /** @param text Sollte auf \n enden */
    public void writeToSchedulerLog(LogCategory category, String text) {
        cppProxy.write_to_scheduler_log(category.getString(), text);
    }

    public String getVersion() {
        return mavenProperties.getVersion();
    }

    public VariableSet getVariables() {
        return cppProxy.variables().getSister();
    }
}
