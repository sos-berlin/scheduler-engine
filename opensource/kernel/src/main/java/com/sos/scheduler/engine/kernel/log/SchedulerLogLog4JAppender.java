package com.sos.scheduler.engine.kernel.log;

import org.apache.log4j.AppenderSkeleton;
import org.apache.log4j.spi.LoggingEvent;

class SchedulerLogLog4JAppender extends AppenderSkeleton {
    private final SchedulerLog schedulerLog;

    SchedulerLogLog4JAppender(SchedulerLog schedulerLog) {
        this.schedulerLog = schedulerLog;
    }

    @Override protected final void append(LoggingEvent e) {
        schedulerLog.write(new LogCategory("log4j." + e.getLoggerName()), e.getMessage() + "\n");
    }

    @Override public final void close() {
    }

    @Override public final boolean requiresLayout() {
        return false;
    }
}
