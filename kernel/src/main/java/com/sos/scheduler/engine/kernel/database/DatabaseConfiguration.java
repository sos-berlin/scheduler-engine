package com.sos.scheduler.engine.kernel.database;

public final class DatabaseConfiguration {
    private final String url;
    private final String taskHistoryTablename;

    
    public DatabaseConfiguration(String url, String taskHistoryTablename) {
        this.url = url;
        this.taskHistoryTablename = taskHistoryTablename;
    }


    public final String getUrl() {
        return url;
    }

    public final String getTaskHistoryTablename() {
        return taskHistoryTablename;
    }
}
