package com.sos.scheduler.engine.kernel.database;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.Subsystem;
import com.sos.scheduler.engine.kernel.cppproxy.DatabaseC;


@ForCpp
public class DatabaseSubsystem implements Subsystem {
    private final DatabaseC cppProxy;
    private final DatabaseConfiguration configuration;


    public DatabaseSubsystem(DatabaseC cppProxy) {
        this.cppProxy = cppProxy;
        configuration = new DatabaseConfiguration(cppProxy.db_name(), cppProxy.job_history_tablename());
    }


    /** Mit Kennwort. */
    public String getUrl() { return cppProxy.db_name(); }

    public DatabaseConfiguration getConfiguration() { return configuration; }
}
