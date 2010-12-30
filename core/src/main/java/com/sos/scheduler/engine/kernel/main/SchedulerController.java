package com.sos.scheduler.engine.kernel.main;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.event.EventSubscriber;
import com.sos.scheduler.engine.kernel.util.Time;


public interface SchedulerController {
    public static final Time terminationTimeout = Time.of(10);

    void loadModule();
    void subscribeEvents(EventSubscriber s);
    void runScheduler(String... arguments);
    void startScheduler(String... arguments);
    Scheduler waitUntilSchedulerIsRunning();
    void waitForTermination(Time timeout);
    void terminateScheduler();
    void terminateAfterException(Throwable x);
    void terminateAndWait();
    int getExitCode();
    //Scheduler getScheduler();
}
