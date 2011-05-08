package com.sos.scheduler.engine.kernel.log;


public interface SchedulerLogger {
    void info(String line);
    void warn(String line);
    void error(String line);
    void debug(String line);
}
