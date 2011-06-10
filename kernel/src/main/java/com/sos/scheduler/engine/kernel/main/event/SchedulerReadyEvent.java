package com.sos.scheduler.engine.kernel.main.event;

import com.sos.scheduler.engine.kernel.main.SchedulerController;


public class SchedulerReadyEvent extends MainEvent {
    private final SchedulerController schedulerController;


    public SchedulerReadyEvent(SchedulerController o) {
        schedulerController = o;
    }


    public final SchedulerController getSchedulerController() {
        return schedulerController;
    }
}
