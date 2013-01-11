package com.sos.scheduler.engine.data.job;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.sos.scheduler.engine.data.event.AbstractEvent;
import com.sos.scheduler.engine.data.folder.JobPath;

public abstract class AbstractTaskEvent extends AbstractEvent implements TaskEvent {
    public static final String taskIdName = "taskId";
    public static final String jobPathName = "jobPath";

    private final TaskId taskId;
    private final JobPath jobPath;

    protected AbstractTaskEvent(TaskId id, JobPath jobPath) {
        this.jobPath = jobPath;
        this.taskId = id;
    }

    @JsonProperty(taskIdName) public final TaskId taskId() {
        return taskId;
    }

    @JsonProperty(jobPathName) public final JobPath jobPath() {
        return jobPath;
    }

    @Override public String toString() {
        return super.toString() +" "+ taskId +" ("+ jobPath +")";
    }
}
