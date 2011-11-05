package com.sos.scheduler.engine.kernel.settings.database;

import javax.annotation.Nullable;

public class DefaultDatabaseSettings implements DatabaseSettings {
    public static final DatabaseSettings singleton = new DefaultDatabaseSettings();

    protected DefaultDatabaseSettings() {}

    /** @return null, wenn C++-Einstellungen beibehalten werden soll. */
    @Override @Nullable public String getHostwarePathOrNull() {
        return null;
    }
}
