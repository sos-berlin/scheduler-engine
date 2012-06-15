package com.sos.scheduler.engine.data.job;

import com.sos.scheduler.engine.data.folder.AbsolutePath;
import org.codehaus.jackson.annotate.JsonCreator;
import org.codehaus.jackson.annotate.JsonProperty;

public class TaskStartedEvent extends TaskEvent {
    @JsonCreator
    public TaskStartedEvent(@JsonProperty("id") TaskId id, @JsonProperty("jobPath") AbsolutePath jobPath) {
        super(id, jobPath);
    }
}
