package com.sos.scheduler.engine.kernel.test;

import java.io.File;

import com.sos.scheduler.engine.kernel.settings.DefaultSettings;
import com.sos.scheduler.engine.kernel.settings.database.DatabaseSettings;
import com.sos.scheduler.engine.kernel.settings.database.DefaultDatabaseSettings;
import com.sos.scheduler.engine.kernel.util.Hostware;

public class TemporaryDatabaseSettings extends DefaultSettings {
    private final File directory;

    public TemporaryDatabaseSettings(File directory) {
        this.directory = directory;
    }

    @Override public DatabaseSettings getDatabaseSettings() {
        return new DefaultDatabaseSettings() {
            @Override public String getHostwarePathOrNull() {
                File databaseFile = new File(directory, "scheduler-database");
                return Hostware.h2DatabasePath(databaseFile);
            }
        };
    }
}
