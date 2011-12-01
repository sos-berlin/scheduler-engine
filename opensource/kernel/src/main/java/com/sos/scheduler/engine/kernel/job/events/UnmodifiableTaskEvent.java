package com.sos.scheduler.engine.kernel.job.events;

import com.sos.scheduler.engine.kernel.job.UnmodifiableTask;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerObject;

public class UnmodifiableTaskEvent extends TaskEvent {
    private final UnmodifiableTask task;

    protected UnmodifiableTaskEvent(UnmodifiableTask task) {
        this.task = task;
    }

    @Override public final SchedulerObject getObject() {
        return task;
    }
}
