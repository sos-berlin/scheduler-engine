package com.sos.scheduler.engine.persistence;

import com.sos.jobscheduler.data.scheduler.SchedulerId;
import java.time.Instant;
import java.util.Date;

public final class SchedulerDatabases {
    private SchedulerDatabases() {}

    public static final String persistenceUnitName = "JobScheduler-Engine";
    public static final String emptyIdInDatabase = "-";

    public static String schedulerIdToDatabase(SchedulerId id) {
        return id.isEmpty()? emptyIdInDatabase : id.string();
    }

    public static SchedulerId schedulerIdFromDatabase(String id) {
        return new SchedulerId(id.equals(emptyIdInDatabase)? "" : id);
    }

    public static Instant databaseToInstant(Date o) {
        return Instant.ofEpochMilli(o.getTime());
    }

    public static org.joda.time.Instant databaseToJodaInstant(Date o) {
        return new org.joda.time.Instant(o.getTime());
    }

    public static Date instantToDatabase(Instant o) {
        return new Date(o.toEpochMilli());
    }

    public static Date instantToDatabase(org.joda.time.ReadableInstant o) {
        return new Date(o.getMillis());
    }
}
