package com.sos.scheduler.engine.data.job;

import com.sos.scheduler.engine.data.folder.JobPath;
import org.codehaus.jackson.annotate.JsonProperty;

public class TaskStartedEvent extends AbstractTaskEvent {
    public TaskStartedEvent(
            @JsonProperty(taskIdName) TaskId id,
            @JsonProperty(jobPathName) JobPath jobPath) {
        super(id, jobPath);
    }
}
