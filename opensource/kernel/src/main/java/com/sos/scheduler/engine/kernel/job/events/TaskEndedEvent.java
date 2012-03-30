package com.sos.scheduler.engine.kernel.job.events;

import com.sos.scheduler.engine.kernel.folder.AbsolutePath;
import com.sos.scheduler.engine.kernel.job.TaskId;

public class TaskEndedEvent extends UnmodifiableTaskEvent {
    public TaskEndedEvent(TaskId id, AbsolutePath jobPath) {
        super(id, jobPath);
    }
}
