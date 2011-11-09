package com.sos.scheduler.engine.kernel.job.events;

import com.sos.scheduler.engine.kernel.event.ObjectEvent;
import com.sos.scheduler.engine.kernel.job.UnmodifiableTask;

public class UnmodifiableTaskEvent extends ObjectEvent {
    private final UnmodifiableTask task;

    public UnmodifiableTaskEvent(UnmodifiableTask task) {
        this.task = task;
    }

    @Override protected UnmodifiableTask getObject() {
        return task;
    }
}
