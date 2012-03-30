package com.sos.scheduler.engine.kernel.job.events;

import com.sos.scheduler.engine.kernel.folder.AbsolutePath;
import com.sos.scheduler.engine.kernel.job.TaskId;

public class UnmodifiableTaskEvent extends TaskEvent {
    private final TaskId id;
    private final AbsolutePath jobPath;

    protected UnmodifiableTaskEvent(TaskId id, AbsolutePath jobPath) {
        this.jobPath = jobPath;
        this.id = id;
    }

    public final TaskId getId() {
        return id;
    }

    public final AbsolutePath getJobPath() {
        return jobPath;
    }
}
