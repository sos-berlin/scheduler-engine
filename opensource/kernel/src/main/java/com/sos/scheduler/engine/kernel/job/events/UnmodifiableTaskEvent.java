package com.sos.scheduler.engine.kernel.job.events;

import com.sos.scheduler.engine.kernel.folder.AbsolutePath;
import com.sos.scheduler.engine.kernel.job.UnmodifiableTask;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerObject;

public class UnmodifiableTaskEvent extends TaskEvent {
    private final UnmodifiableTask task;
    private final AbsolutePath jobPath;


    protected UnmodifiableTaskEvent(UnmodifiableTask task) {
        this.task = task;
        jobPath = task.getJob().getPath();
    }

    @Override public final SchedulerObject getObject() {
        return task;
    }

    public AbsolutePath getJobPath() {
        return jobPath;
    }
}
