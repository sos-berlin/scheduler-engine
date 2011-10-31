package com.sos.scheduler.engine.kernel;

import static com.google.common.base.Objects.firstNonNull;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.loadXml;

import java.util.ArrayList;
import java.util.List;

import javax.annotation.Nullable;

import org.apache.log4j.Logger;
import org.w3c.dom.Element;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.command.CommandHandler;
import com.sos.scheduler.engine.kernel.command.CommandSubsystem;
import com.sos.scheduler.engine.kernel.command.HasCommandHandlers;
import com.sos.scheduler.engine.kernel.command.UnknownCommandException;
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC;
import com.sos.scheduler.engine.kernel.database.DatabaseSubsystem;
import com.sos.scheduler.engine.kernel.event.OperationCollector;
import com.sos.scheduler.engine.kernel.event.OperationExecutor;
import com.sos.scheduler.engine.kernel.event.EventSubsystem;
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem;
import com.sos.scheduler.engine.kernel.job.JobSubsystem;
import com.sos.scheduler.engine.kernel.log.LogCategory;
import com.sos.scheduler.engine.kernel.log.LogSubsystem;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.log.SchedulerLog;
import com.sos.scheduler.engine.kernel.main.SchedulerStateHandler;
import com.sos.scheduler.engine.kernel.order.OrderSubsystem;
import com.sos.scheduler.engine.kernel.plugin.PluginSubsystem;
import com.sos.scheduler.engine.kernel.schedulerevent.SchedulerCloseEvent;

@ForCpp
public final class Scheduler implements HasPlatform, Sister {
    private static final Logger logger = Logger.getLogger(Scheduler.class);

    private final MavenProperties mavenProperties = new MavenProperties(Scheduler.class);
    private final SpoolerC cppProxy;
    private final SchedulerStateHandler schedulerStateHandler;
    private final PrefixLog log;
    private final Platform platform;
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

    @ForCpp public Scheduler(SpoolerC cppProxy, @Nullable SchedulerStateHandler schedulerStateHandler) {
        this.cppProxy = cppProxy;
        this.schedulerStateHandler = firstNonNull(schedulerStateHandler, SchedulerStateHandler.empty);
        cppProxy.setSister(this);
        log = new PrefixLog(cppProxy.log());
        platform = new Platform(log);
        operationExecutor = new OperationExecutor(log);

        logSubsystem = new LogSubsystem(new SchedulerLog(this.cppProxy));
        databaseSubsystem = new DatabaseSubsystem(this.cppProxy.db());
        eventSubsystem = new EventSubsystem(platform, operationExecutor);
        folderSubsystem = new FolderSubsystem(this.cppProxy.folder_subsystem());
        jobSubsystem = new JobSubsystem(platform, this.cppProxy.job_subsystem());
        orderSubsystem = new OrderSubsystem(platform, this.cppProxy.order_subsystem());
        pluginSubsystem = new PluginSubsystem(this, eventSubsystem);
        commandSubsystem = new CommandSubsystem(getCommandHandlers(ImmutableList.of(pluginSubsystem)));

        initializeThreadLock();
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
        return log;
    }

    @Override public void onCppProxyInvalidated() {}

    @ForCpp public void onClose() {
        try {
            eventSubsystem.report(new SchedulerCloseEvent());

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
    }

    @ForCpp public void onLoad(String configurationXml) {
        onLoad(loadXml(configurationXml).getDocumentElement());
    }

    private void onLoad(Element configElement) {
        pluginSubsystem.load(configElement);
        schedulerStateHandler.onSchedulerStarted(this);
    }

    @ForCpp public void onActivate() {
        logSubsystem.activate();
        pluginSubsystem.activate();
    }

    @ForCpp public void onActivated() {
        schedulerStateHandler.onSchedulerActivated();
    }

    @ForCpp public void onEnteringSleepState() {
        operationExecutor.execute();
    }

    public void terminate() {
        cppProxy.cmd_terminate();
    }

    private void initializeThreadLock() {
        if (!isStartedByJava()) { // Wenn wir ein schedulerStateHandler haben, ist der Scheduler über Java (CppScheduler.main) aufgerufen worden. Dort wird die Sperre gesetzt.
            threadLock();
            threadInitiallyLocked = true;
        }
    }

    private boolean isStartedByJava() {
        return schedulerStateHandler != SchedulerStateHandler.empty;
    }

    @ForCpp public void threadLock() {
        CppProxy.threadLock.lock();
    }

    @ForCpp public void threadUnlock() {
        CppProxy.threadLock.unlock();
    }

    public String executeXml(String xml) {
        return cppProxy.execute_xml(xml);
    }

    /** Nur für C++, zur Ausführung eines Kommandos in Java */
    @ForCpp public String javaExecuteXml(String xml) {
        try {
            return commandSubsystem.executeXml(xml);
        } catch (UnknownCommandException x) {
            log.warn(x.toString());
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

    public OperationCollector getOperationCollector() {
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
}
