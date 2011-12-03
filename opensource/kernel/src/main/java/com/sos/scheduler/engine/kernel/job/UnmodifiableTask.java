package com.sos.scheduler.engine.kernel.job;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerObject;

@ForCpp
public interface UnmodifiableTask extends SchedulerObject {
    UnmodifiableJob getJob();
}
