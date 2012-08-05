package com.sos.scheduler.engine.plugins.databasequery;

import com.sos.scheduler.engine.kernel.command.Result;
import com.sos.scheduler.engine.data.database.TaskHistoryEntity;
import java.util.Collection;

class TaskHistoryEntriesResult implements Result {
    private final Collection<TaskHistoryEntity> entities;   // Nimmt TypedQuery.getResultList auf

    TaskHistoryEntriesResult(Collection<TaskHistoryEntity> entities) {
        this.entities = entities;
    }

    final Collection<TaskHistoryEntity> getTaskHistoryEntries() {
        return entities;
    }
}
