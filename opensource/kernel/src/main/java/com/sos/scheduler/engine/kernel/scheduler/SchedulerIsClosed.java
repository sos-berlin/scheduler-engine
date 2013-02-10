package com.sos.scheduler.engine.kernel.scheduler;

import com.google.inject.ImplementedBy;
import com.sos.scheduler.engine.kernel.Scheduler;

@ImplementedBy(Scheduler.class)
public interface SchedulerIsClosed {
    boolean isClosed();
}
