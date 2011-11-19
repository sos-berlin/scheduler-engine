package com.sos.scheduler.engine.kernel.settings;

import com.sos.scheduler.engine.kernel.settings.database.DatabaseSettings;
import com.sos.scheduler.engine.kernel.settings.database.DefaultDatabaseSettings;

public class DefaultSettings implements Settings {
    public static final DefaultSettings singleton = new DefaultSettings();

    protected DefaultSettings() {}

    public DatabaseSettings getDatabaseSettings() {
        return DefaultDatabaseSettings.singleton;
    }
}
