package com.sos.scheduler.engine.kernel.job.events;

import com.sos.scheduler.engine.kernel.folder.Path;
import com.sos.scheduler.engine.kernel.job.UnmodifiableTask;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerObject;

public class UnmodifiableTaskEvent extends TaskEvent {
    private final UnmodifiableTask task;
    private final Path jobPath;


    protected UnmodifiableTaskEvent(UnmodifiableTask task) {
        this.task = task;
        jobPath = task.getJob().getPath();
    }

    @Override public final SchedulerObject getObject() {
        return task;
    }

    public Path getJobPath() {
        return jobPath;
    }
}
