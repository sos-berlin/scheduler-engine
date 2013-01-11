package com.sos.scheduler.engine.data.job;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.sos.scheduler.engine.data.folder.JobPath;

public class TaskEndedEvent extends AbstractTaskEvent {
    public TaskEndedEvent(@JsonProperty(taskIdName) TaskId id, @JsonProperty(jobPathName) JobPath jobPath) {
        super(id, jobPath);
    }
}
