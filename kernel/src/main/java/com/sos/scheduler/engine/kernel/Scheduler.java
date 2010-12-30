package com.sos.scheduler.engine.kernel;

import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.job.JobSubsystem;
import com.sos.scheduler.engine.kernel.job.Job;
import com.sos.scheduler.engine.kernel.cppproxy.SpoolerC;
import com.sos.scheduler.engine.kernel.event.EventSubsystem;
import com.sos.scheduler.engine.kernel.main.MainContext;
import com.sos.scheduler.engine.kernel.monitorwindow.Monitor;
import com.sos.scheduler.engine.kernel.order.OrderSubsystem;
import com.sos.scheduler.kernel.cplusplus.runtime.Sister;
import com.sos.scheduler.kernel.cplusplus.runtime.ThreadLock;
import com.sos.scheduler.kernel.cplusplus.runtime.annotation.ForCpp;
import org.apache.log4j.Logger;


@ForCpp
public class Scheduler implements HasPlatform, Sister  // extends SchedulerObject
{
    private static final Logger logger = Logger.getLogger(Scheduler.class);

    private final SpoolerC spoolerC;
    private final MainContext mainContext;
    private final Platform platform;
    private final PrefixLog log;
    private JobSubsystem jobSubsystem;
    private OrderSubsystem orderSubsystem;
    private EventSubsystem eventSubsystem;
    private Monitor monitor;
    private boolean threadInitiallyLocked = false;
    
    
    public Scheduler(SpoolerC spoolerC, MainContext mainContext) {
        this.spoolerC = spoolerC;
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

    @Override public Platform getPlatform() { return platform; }
    @Override public PrefixLog log() { return log; }

    
    // Sister
    
    @Override public void onCppProxyInvalidated() {}


    public void onClose() {
        try {
            if (monitor != null)  
                try { monitor.close(); }
                catch (Exception x) { log().error("monitor.close(): " + x); }

            if (eventSubsystem != null)
                eventSubsystem.report(new SchedulerCloseEvent(this));
        }
        finally {
            if (threadInitiallyLocked) {
                threadUnlock();     //TODO Sperre wird zu früh freigegeben, der Scheduler läuft ja noch. Lösung: Start über Java mit CppScheduler.run()
                threadInitiallyLocked = false;
            }
        }
    }
    

    public void onLoad() {
        eventSubsystem = new EventSubsystem(platform);
        jobSubsystem = new JobSubsystem(platform, spoolerC.job_subsystem());
        orderSubsystem = new OrderSubsystem(platform, spoolerC.order_subsystem());
        if (mainContext != null)  mainContext.setScheduler(this);
    }


    public void onActivate() {
        if (mainContext != null)  mainContext.onSchedulerActivated();
    }


    public void terminate() {
        spoolerC.cmd_terminate();
    }


    public final void threadLock() {
        ThreadLock.lock();
    }


    public final void threadUnlock() {
        ThreadLock.unlock();
    }

    public String executeXml(String xml) {
        return spoolerC.execute_xml(xml);
    }
    
    public Object getMainContext() { return mainContext; }
    public EventSubsystem getEventSubsystem() { return eventSubsystem; }
    public JobSubsystem getJobSubsystem() { return jobSubsystem; }
    public OrderSubsystem getOrderSubsystem() { return orderSubsystem; }

    public String getHttpUrl() { return spoolerC.http_url(); }

    public int getTcpPort() { return spoolerC.tcp_port(); }
}
