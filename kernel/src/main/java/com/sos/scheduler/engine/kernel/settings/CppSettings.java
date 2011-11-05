package com.sos.scheduler.engine.kernel.settings;

import com.sos.scheduler.engine.kernel.cppproxy.SettingsC;
import com.sos.scheduler.engine.kernel.settings.database.DatabaseSettings;

public final class CppSettings {
    private final SettingsC cppProxy;

    public CppSettings(SettingsC cppProxy) {
        this.cppProxy = cppProxy;
    }

    public void setCppSettings(Settings s) {
        setDatabaseCppSettings(s.getDatabaseSettings());
    }

    private void setDatabaseCppSettings(DatabaseSettings s) {
        if (s.getHostwarePathOrNull() != null)
            cppProxy.set_db_name(s.getHostwarePathOrNull());
    }
}
