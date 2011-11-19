package com.sos.scheduler.engine.kernel.job.events;

import com.sos.scheduler.engine.kernel.SchedulerObject;
import com.sos.scheduler.engine.kernel.event.ObjectEvent;
import com.sos.scheduler.engine.kernel.job.UnmodifiableTask;

public abstract class TaskEvent extends ObjectEvent {
    @Override protected final SchedulerObject getObject() {
        return getTask();
    }

    protected abstract UnmodifiableTask getTask();
}
