package com.sos.scheduler.engine.kernel;

import com.sos.scheduler.engine.kernel.event.ObjectEvent;


public class SchedulerCloseEvent extends ObjectEvent<Scheduler>
{
    public SchedulerCloseEvent(Scheduler o) {
        super(o);
    }
}
