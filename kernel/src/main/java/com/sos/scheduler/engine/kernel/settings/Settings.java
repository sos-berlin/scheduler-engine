package com.sos.scheduler.engine.kernel.settings;

import com.sos.scheduler.engine.kernel.settings.database.DatabaseSettings;

public interface Settings {
    DatabaseSettings getDatabaseSettings();
}
