package com.sos.scheduler.engine.kernel.main.event;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.main.SchedulerController;

public class SchedulerReadyEvent extends MainEvent {
    private final SchedulerController schedulerController;
    private final Scheduler scheduler;

    public SchedulerReadyEvent(SchedulerController o, Scheduler scheduler) {
        schedulerController = o;
        this.scheduler = scheduler;
    }

    public final SchedulerController getSchedulerController() {
        return schedulerController;
    }

    public final Scheduler getScheduler() {
        return scheduler;
    }
}
