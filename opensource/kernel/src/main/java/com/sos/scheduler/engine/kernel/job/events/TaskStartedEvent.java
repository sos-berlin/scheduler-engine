package com.sos.scheduler.engine.kernel.job.events;

import com.sos.scheduler.engine.kernel.folder.AbsolutePath;
import com.sos.scheduler.engine.kernel.job.TaskId;

public class TaskStartedEvent extends UnmodifiableTaskEvent {
    public TaskStartedEvent(TaskId id, AbsolutePath jobPath) {
        super(id, jobPath);
    }
}
