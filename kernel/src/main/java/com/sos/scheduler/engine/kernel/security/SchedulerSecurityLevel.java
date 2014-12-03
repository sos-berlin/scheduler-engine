package com.sos.scheduler.engine.kernel.security;

public enum SchedulerSecurityLevel {
    none("none"),
    signal("signal"),
    info("info"),
    no_add("no_add"),
    all("all");

    private final String cppName;

    SchedulerSecurityLevel(String cppName) {
        this.cppName = cppName;
    }

    public String cppName() {
        return cppName;
    }
}
