package com.sos.scheduler.engine.data.job;

import com.sos.scheduler.engine.data.folder.JobPath;
import org.codehaus.jackson.annotate.JsonCreator;
import org.codehaus.jackson.annotate.JsonProperty;

public class TaskEndedEvent extends TaskEvent {
    @JsonCreator
    public TaskEndedEvent(@JsonProperty("id") TaskId id, @JsonProperty("jobPath") JobPath jobPath) {
        super(id, jobPath);
    }
}
