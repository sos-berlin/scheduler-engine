package com.sos.scheduler.engine.kernel.job;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.eventbus.EventSource;
import com.sos.scheduler.engine.kernel.order.OrderId;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.kernel.scheduler.HasPlatform;

@ForCpp
public interface UnmodifiableTask extends EventSource, HasPlatform {
	int getId();
    UnmodifiableJob getJob();
    UnmodifiableOrder getOrder();
}
