package com.sos.scheduler.engine.kernel.log;

public enum SchedulerLogLevel {
    debug(-1, "debug"),
    info(0, "info"),
    warn(1, "warn"),
    error(2, "error");

    private final int number;
    private final String cppName;

    SchedulerLogLevel(int n, String cppName) {
        this.number = n;
        this.cppName = cppName;
    }

    public int getNumber() {
        return number;
    }

    String getCppName() {
        return cppName;
    }
}
