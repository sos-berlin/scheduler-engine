package com.sos.scheduler.engine.data.job;

import com.sos.scheduler.engine.data.folder.JobPath;
import org.codehaus.jackson.annotate.JsonProperty;

public class TaskEndedEvent extends AbstractTaskEvent {
    public TaskEndedEvent(@JsonProperty(taskIdName) TaskId id, @JsonProperty(jobPathName) JobPath jobPath) {
        super(id, jobPath);
    }
}
