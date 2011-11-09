package com.sos.scheduler.engine.kernel.job.events;

import com.sos.scheduler.engine.kernel.event.ObjectEvent;
import com.sos.scheduler.engine.kernel.job.UnmodifiableTask;

public class TaskEvent extends ObjectEvent {
    private final UnmodifiableTask task;

    public TaskEvent(UnmodifiableTask task) {
        this.task = task;
    }

    @Override protected final UnmodifiableTask getObject() {
        return task;
    }
}
