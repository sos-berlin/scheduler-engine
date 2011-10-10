package com.sos.scheduler.engine.kernel.schedulerevent;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.engine.kernel.SchedulerObject;
import com.sos.scheduler.engine.kernel.event.ObjectEvent;

public abstract class SchedulerEvent extends ObjectEvent {
    private final Scheduler scheduler;

    protected SchedulerEvent(Scheduler o) {
        scheduler = o;
    }

    @Override protected final SchedulerObject getObject() {
        return scheduler;
    }
}
