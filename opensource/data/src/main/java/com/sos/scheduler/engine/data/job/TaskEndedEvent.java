package com.sos.scheduler.engine.data.job;

import com.sos.scheduler.engine.data.folder.AbsolutePath;

public class TaskEndedEvent extends TaskEvent {
    public TaskEndedEvent(TaskId id, AbsolutePath jobPath) {
        super(id, jobPath);
    }
}
