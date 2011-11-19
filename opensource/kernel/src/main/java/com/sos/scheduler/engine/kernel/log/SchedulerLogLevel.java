package com.sos.scheduler.engine.kernel.log;

public enum SchedulerLogLevel {
    debug("debug"),
    info("info"),
    warn("warn"),
    error("error");

    private final String cppName;

    SchedulerLogLevel(String cppName) {
        this.cppName = cppName;
    }

    String getCppName() {
        return cppName;
    }
}
