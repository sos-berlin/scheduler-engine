package com.sos.scheduler.engine.persistence;

import com.sos.scheduler.engine.data.scheduler.SchedulerId;
import org.joda.time.DateTime;
import org.joda.time.DateTimeZone;

import java.util.Date;

import static org.joda.time.DateTimeZone.UTC;

public final class SchedulerDatabases {
    private SchedulerDatabases() {}

    public static final String persistenceUnitName = "JobScheduler-Engine";
    public static final String emptyIdInDatabase = "-";
    public static final DateTimeZone databaseTimeZone = UTC;

    public static String schedulerIdToDatabase(SchedulerId id) {
        return id.isEmpty()? emptyIdInDatabase : id.asString();
    }

    public static SchedulerId schedulerIdFromDatabase(String id) {
        return new SchedulerId(id.equals(emptyIdInDatabase)? "" : id);
    }

    public static DateTime databaseToDateTime(Date o) {
        return new DateTime(o.getTime(), databaseTimeZone);
    }

    public static Date dateTimeToDatabase(DateTime o) {
        return new Date(o.getMillis());
    }
}
