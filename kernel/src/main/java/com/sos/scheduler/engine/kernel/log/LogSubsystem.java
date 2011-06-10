package com.sos.scheduler.engine.kernel.log;

import com.sos.scheduler.engine.kernel.Subsystem;
import org.apache.log4j.Appender;
import org.apache.log4j.Logger;


public final class LogSubsystem implements Subsystem {
    private final Appender appender;


    public LogSubsystem(SchedulerLog schedulerLog) {
        appender = new SchedulerLogLog4JAppender(schedulerLog);
    }


    public void close() {
        Logger.getRootLogger().removeAppender(appender);
    }


    public void activate() {
        Logger.getRootLogger().addAppender(appender);
    }
}
