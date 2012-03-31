package com.sos.scheduler.engine.data.database;

import com.sos.scheduler.engine.data.scheduler.SchedulerId;

public final class SchedulerDatabases {
    public static final String emptyIdInDatabase = "-";

    public static String idForDatabase(SchedulerId id) {
        return id.isEmpty()? emptyIdInDatabase : id.getString();
    }

    private SchedulerDatabases() {}
}
