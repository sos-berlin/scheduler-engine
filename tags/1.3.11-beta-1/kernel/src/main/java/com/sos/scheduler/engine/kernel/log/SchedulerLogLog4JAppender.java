package com.sos.scheduler.engine.kernel.log;

import org.apache.log4j.AppenderSkeleton;
import org.apache.log4j.spi.LoggingEvent;


public class SchedulerLogLog4JAppender extends AppenderSkeleton {
    private final SchedulerLog schedulerLog;

    public SchedulerLogLog4JAppender(SchedulerLog schedulerLog) {
        this.schedulerLog = schedulerLog;
    }

    @Override protected void append(LoggingEvent e) {
        schedulerLog.write(new LogCategory("log4j." + e.getLoggerName()), e.getMessage() + "\n");
    }

    @Override public void close() {}

    @Override public boolean requiresLayout() {
        return false;
    }
}
