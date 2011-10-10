package com.sos.scheduler.engine.kernel.schedulerevent;

import com.sos.scheduler.engine.kernel.Scheduler;

/** Wenn das Java_subsystem geschlossen wird. */
public class SchedulerCloseEvent extends SchedulerEvent {
    public SchedulerCloseEvent(Scheduler o) {
        super(o);
    }
}
