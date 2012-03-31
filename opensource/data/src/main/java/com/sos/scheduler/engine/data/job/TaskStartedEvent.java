package com.sos.scheduler.engine.data.job;

import com.sos.scheduler.engine.data.folder.AbsolutePath;

public class TaskStartedEvent extends TaskEvent {
    public TaskStartedEvent(TaskId id, AbsolutePath jobPath) {
        super(id, jobPath);
    }
}
