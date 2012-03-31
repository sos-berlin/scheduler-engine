package com.sos.scheduler.engine.data.job;

import com.sos.scheduler.engine.data.event.AbstractEvent;
import com.sos.scheduler.engine.data.folder.AbsolutePath;

public abstract class TaskEvent extends AbstractEvent {
    private final TaskId id;
    private final AbsolutePath jobPath;

    protected TaskEvent(TaskId id, AbsolutePath jobPath) {
        this.jobPath = jobPath;
        this.id = id;
    }

    public final TaskId getId() {
        return id;
    }

    public final AbsolutePath getJobPath() {
        return jobPath;
    }

    @Override public String toString() {
        return super.toString() +" "+ id +" ("+ jobPath +")";
    }
}
