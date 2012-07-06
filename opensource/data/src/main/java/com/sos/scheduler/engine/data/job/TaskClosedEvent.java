package com.sos.scheduler.engine.data.job;

import com.sos.scheduler.engine.data.folder.JobPath;
import org.codehaus.jackson.annotate.JsonCreator;
import org.codehaus.jackson.annotate.JsonProperty;

/** Nach {@link TaskEndedEvent} und nachdem das Task-Objekt geschlossen worden ist. */
public class TaskClosedEvent extends TaskEvent {
    @JsonCreator
    public TaskClosedEvent(@JsonProperty("id") TaskId id, @JsonProperty("jobPath") JobPath jobPath) {
        super(id, jobPath);
    }
}
