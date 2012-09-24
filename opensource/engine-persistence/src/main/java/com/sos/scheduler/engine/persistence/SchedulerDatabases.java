package com.sos.scheduler.engine.persistence;

import com.sos.scheduler.engine.data.scheduler.SchedulerId;

public final class SchedulerDatabases {
    public static final String persistenceUnitName = "JobScheduler-Engine";
    public static final String emptyIdInDatabase = "-";

    public static String idForDatabase(SchedulerId id) {
        return id.isEmpty()? emptyIdInDatabase : id.asString();
    }

    public static SchedulerId schedulerIdFromDatabase(String id) {
        return new SchedulerId(id.equals(emptyIdInDatabase)? "" : id);
    }

    private SchedulerDatabases() {}
}
