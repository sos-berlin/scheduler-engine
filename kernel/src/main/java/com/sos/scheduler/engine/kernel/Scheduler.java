package com.sos.scheduler.engine.kernel;

import com.sos.scheduler.engine.kernel.command.CommandHandler;
import com.sos.scheduler.engine.kernel.database.DatabaseSubsystem;
import java.util.ArrayList;
import java.util.List;
import com.sos.scheduler.engine.kernel.command.CommandSubsystem;
import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC;
import com.sos.scheduler.engine.kernel.job.JobSubsystem;
import com.sos.scheduler.engine.kernel.event.EventSubsystem;
import com.sos.scheduler.engine.kernel.log.LogCategory;
import com.sos.scheduler.engine.kernel.log.LogSubsystem;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.log.SchedulerLog;
import com.sos.scheduler.engine.kernel.main.MainContext;
import com.sos.scheduler.engine.kernel.monitorwindow.Monitor;
import com.sos.scheduler.engine.kernel.order.OrderSubsystem;
import com.sos.scheduler.engine.kernel.plugin.PlugInSubsystem;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.command.HasCommandHandlers;
import com.sos.scheduler.engine.kernel.command.UnknownCommandException;
import org.apache.log4j.Logger;
import org.w3c.dom.Element;
import static com.sos.scheduler.engine.kernel.util.XmlUtils.*;


@ForCpp
public final class Scheduler implements HasPlatform, Sister { // extends SchedulerObject
    private static final Logger logger = Logger.getLogger(Scheduler.class);

    private final SpoolerC cppProxy;
    private final MainContext mainContext;
    private final Platform platform;
    private final PrefixLog log;
    private MavenProperties mavenProperties = new MavenProperties(getClass());
    private LogSubsystem logSubsystem;
    private DatabaseSubsystem databaseSubsystem;
    private PlugInSubsystem plugInSubsystem;
    private JobSubsystem jobSubsystem;
    private OrderSubsystem orderSubsystem;
    private EventSubsystem eventSubsystem;
    private List<Subsystem> subsystems = new ArrayList<Subsystem>();
    private CommandSubsystem commandSubsystem;
    private Monitor monitor;
    private boolean threadInitiallyLocked = false;
    
    
    public Scheduler(SpoolerC spoolerC, MainContext mainContext) {
        this.cppProxy = spoolerC;
        this.mainContext = mainContext;
        spoolerC.setSister(this);
        log = new PrefixLog(spoolerC.log());
        platform = new Platform(log);

        if (mainContext == null) {  // Wenn wir ein mainContext haben, ist der Scheduler über Java (CppScheduler.main) aufgerufen worden. Dort wird die Sperre gesetzt.
            threadLock();
            threadInitiallyLocked = true;
        }
    }


    // HasPlatform

    @Override public Platform getPlatform() {
        return platform;
    }

    @Override public PrefixLog log() {
        return log;
    }

    
    // Sister
    
    @Override public void onCppProxyInvalidated() {}


    public void onClose() {
        try {
            if (monitor != null)  
                try { monitor.close(); }
                catch (Exception x) { log().error("monitor.close(): " + x); }

            if (eventSubsystem != null)
                eventSubsystem.report(new SchedulerCloseEvent(this));
            
            try { logSubsystem.close(); }
            catch (Exception x) { log().error("logSubsystem.close(): " + x); }
        }
        finally {
            if (threadInitiallyLocked) {
                threadUnlock();     //TODO Sperre wird zu früh freigegeben, der Scheduler läuft ja noch. Lösung: Start über Java mit CppScheduler.run()
                threadInitiallyLocked = false;
            }
        }
        
    }
    

    public void onLoad(String configurationXml) {
        Element configElement = loadXml(configurationXml).getDocumentElement();
        addSubsystems(configElement);
        if (mainContext != null)  mainContext.setScheduler(this);
    }


    private void addSubsystems(Element configElement) {
        logSubsystem = new LogSubsystem(new SchedulerLog(cppProxy));
        subsystems.add(logSubsystem);

        databaseSubsystem = new DatabaseSubsystem(cppProxy.db());
        subsystems.add(databaseSubsystem);

        eventSubsystem = new EventSubsystem(platform);
        subsystems.add(eventSubsystem);

        jobSubsystem = new JobSubsystem(platform, cppProxy.job_subsystem());
        subsystems.add(jobSubsystem);

        orderSubsystem = new OrderSubsystem(platform, cppProxy.order_subsystem());
        subsystems.add(orderSubsystem);

        plugInSubsystem = new PlugInSubsystem(this);
        plugInSubsystem.load(configElement);
        subsystems.add(plugInSubsystem);

        commandSubsystem = new CommandSubsystem(getCommandHandlers(subsystems));
        subsystems.add(commandSubsystem);
    }


    private Iterable<CommandHandler> getCommandHandlers(Iterable<?> objects) {
        List<CommandHandler> result = new ArrayList<CommandHandler>();
        for (Object o: objects)
            if (o instanceof HasCommandHandlers)
                result.addAll(((HasCommandHandlers)o).getCommandHandlers());
        return result;
    }

    
    public void onActivate() {
        logSubsystem.activate();
        plugInSubsystem.activate();
        if (mainContext != null)  mainContext.onSchedulerActivated();
    }


    public void terminate() {
        cppProxy.cmd_terminate();
    }


    public void threadLock() {
        CppProxy.threadLock.lock();
    }


    public void threadUnlock() {
        CppProxy.threadLock.unlock();
    }

    public String executeXml(String xml) {
        return cppProxy.execute_xml(xml);
    }

    /** Nur für C++, zur Ausführung eines Kommandos in Java */
    public String javaExecuteXml(String xml) {
        try {
            return commandSubsystem.executeXml(xml);
        } catch (UnknownCommandException x) {
            log.warn(x.toString());
            return "UNKNOWN_COMMAND";   // Siehe command_error.cxx, für ordentliche Meldung SCHEDULER-105, bis Java die selbst liefert kann.
        }
    }

    public Object getMainContext() { return mainContext; }
    public DatabaseSubsystem getDatabaseSubsystem() { return databaseSubsystem; }
    public EventSubsystem getEventSubsystem() { return eventSubsystem; }
    public JobSubsystem getJobSubsystem() { return jobSubsystem; }
    public OrderSubsystem getOrderSubsystem() { return orderSubsystem; }

    public String getSchedulerId() {
        return cppProxy.id();
    }

    public String getClusterMemberId() {
        return cppProxy.cluster_member_id();
    }
    
    public String getHttpUrl() { return cppProxy.http_url(); }

    public int getTcpPort() { return cppProxy.tcp_port(); }
    public String getHostname() { return cppProxy.hostname(); }
    public String getHostnameLong() { return cppProxy.hostname_complete(); }

    public void callCppAndDoNothing() { cppProxy.tcp_port(); }

    /** @param text Sollte auf \n enden */
    public void writeToSchedulerLog(LogCategory category, String text) {
        cppProxy.write_to_scheduler_log(category.getString(), text);
    }

    public String getVersion() {
        return mavenProperties.getVersion();
    }
}
