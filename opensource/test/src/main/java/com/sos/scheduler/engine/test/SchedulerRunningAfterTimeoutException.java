package com.sos.scheduler.engine.test;

import org.joda.time.Duration;

class SchedulerRunningAfterTimeoutException extends RuntimeException {
    SchedulerRunningAfterTimeoutException(Duration timeout) {
        super("Scheduler has not been terminated within "+timeout);
    }
}
