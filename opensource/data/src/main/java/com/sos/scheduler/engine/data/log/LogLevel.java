package com.sos.scheduler.engine.data.log;

public enum LogLevel {
    @Deprecated debug9(-9),
    @Deprecated debug8(-8),
    @Deprecated debug7(-7),
    @Deprecated debug6(-6),
    @Deprecated debug5(-5),
    @Deprecated debug4(-4),
    debug3(-3),
    @Deprecated debug2(-2),
    debug1(-1),
    info(0),
    warning(1),
    error(2);

    private final int cppLogLevel;

    LogLevel(int cppLogLevel) {
        this.cppLogLevel = cppLogLevel;
    }

    public static LogLevel ofCpp(int cppLogLevel) {
        for (LogLevel o: values())
            if (o.cppLogLevel == cppLogLevel) return o;
        throw new RuntimeException("Unknown C++ log_level: "+cppLogLevel);
    }
}
