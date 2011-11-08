package com.sos.scheduler.engine.kernel.test;

import com.sos.scheduler.engine.kernel.util.Time;

class SchedulerRunningAfterTimeoutException extends RuntimeException {
    SchedulerRunningAfterTimeoutException(Time timeout) {
        super("Scheduler has not been terminated within "+timeout);
    }
}
