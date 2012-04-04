package com.sos.scheduler.engine.kernel.job;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.data.job.TaskId;
import com.sos.scheduler.engine.eventbus.EventSource;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;

@ForCpp
public interface UnmodifiableTask extends EventSource {
	TaskId getId();
    UnmodifiableJob getJob();
    UnmodifiableOrder getOrderOrNull();
}
