package com.sos.scheduler.engine.kernel.job.events;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.event.ObjectEvent;
import com.sos.scheduler.engine.kernel.job.UnmodifiableTask;

@ForCpp public class TaskEndedEvent extends ObjectEvent {
    private final UnmodifiableTask task;

    @ForCpp public TaskEndedEvent(UnmodifiableTask task) {
        this.task = task;
    }

    @Override protected UnmodifiableTask getObject() {
        return task;
    }
}
