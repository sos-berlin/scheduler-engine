package com.sos.scheduler.engine.plugins.databasequery;

import com.sos.scheduler.engine.kernel.command.Result;
import com.sos.scheduler.engine.kernel.database.entity.TaskHistoryEntity;
import java.util.Collection;


public class TaskHistoryEntriesResult implements Result {
    private final Collection<TaskHistoryEntity> entities;


    public TaskHistoryEntriesResult(Collection<TaskHistoryEntity> entities) {
        this.entities = entities;
    }


    public final Collection<TaskHistoryEntity> getTaskHistoryEntries() {
        return entities;
    }
}
