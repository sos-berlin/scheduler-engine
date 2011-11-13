package com.sos.scheduler.engine.kernel.job.events;

import com.sos.scheduler.engine.kernel.job.UnmodifiableTask;

public class UnmodifiableTaskEvent extends TaskEvent {
    private final UnmodifiableTask task;

    protected UnmodifiableTaskEvent(UnmodifiableTask task) {
        this.task = task;
    }

    @Override protected UnmodifiableTask getTask() {
        return task;
    }
}
