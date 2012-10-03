package com.sos.scheduler.engine.plugins.databasequery;

import com.sos.scheduler.engine.data.job.TaskHistoryEntry;
import com.sos.scheduler.engine.kernel.command.Result;

import java.util.Collection;

class TaskHistoryEntriesResult implements Result {
    private final Collection<TaskHistoryEntry> taskHistoryEntries;   // Nimmt TypedQuery.getResultList auf

    TaskHistoryEntriesResult(Collection<TaskHistoryEntry> o) {
        this.taskHistoryEntries = o;
    }

    final Collection<TaskHistoryEntry> getTaskHistoryEntries() {
        return taskHistoryEntries;
    }
}
