package com.sos.scheduler.engine.data.job;

import com.sos.scheduler.engine.data.event.AbstractEvent;
import com.sos.scheduler.engine.data.folder.JobPath;

public abstract class TaskEvent extends AbstractEvent {
    private final TaskId id;
    private final JobPath jobPath;

    protected TaskEvent(TaskId id, JobPath jobPath) {
        this.jobPath = jobPath;
        this.id = id;
    }

    public final TaskId getId() {
        return id;
    }

    public final JobPath getJobPath() {
        return jobPath;
    }

    @Override public String toString() {
        return super.toString() +" "+ id +" ("+ jobPath +")";
    }
}
