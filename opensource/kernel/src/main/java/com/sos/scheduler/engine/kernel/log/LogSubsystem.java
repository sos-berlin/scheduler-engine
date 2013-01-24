package com.sos.scheduler.engine.kernel.log;

import com.sos.scheduler.engine.kernel.scheduler.Subsystem;

public final class LogSubsystem implements Subsystem {
//    private static final Logger logger = LoggerFactory.getLogger(LogSubsystem.class);

//    private final Appender appender;

    public LogSubsystem(SchedulerLog schedulerLog) {
//        appender = new SchedulerLogLog4JAppender(schedulerLog);
    }

    public void close() {
//        Logger l = Logger.getRootLogger();
//        if (l != null) {
//            logToAppender(Level.INFO, "Closed");
//            Logger.getRootLogger().removeAppender(appender);
//        }
    }

    public void activate() {
//        Logger.getRootLogger().addAppender(appender);
//        logToAppender(Level.INFO, "Activated");
    }

//    private void logToAppender(Level level, String message) {
//        appender.doAppend(new LoggingEvent(logger.getName(), logger, level, message, null));
//    }
}
