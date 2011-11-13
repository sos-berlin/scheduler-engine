package com.sos.scheduler.engine.kernel.main;

import com.sos.scheduler.engine.eventbus.EventBus;
import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.settings.Settings;
import com.sos.scheduler.engine.kernel.util.Time;

/** Steuerung für den C++-Scheduler in einem eigenen nebenläufigen Thread. */
public interface SchedulerController {
    void setSettings(Settings o);
    void startScheduler(String... arguments);
    Scheduler waitUntilSchedulerIsRunning();
    void waitUntilSchedulerState(SchedulerState s);
    //SchedulerState getSchedulerState();
    boolean tryWaitForTermination(Time timeout);
    void terminateScheduler();
    void terminateAfterException(Throwable x);
    void terminateAndWait();
    int exitCode();
    EventBus getEventBus();
}
