package com.sos.scheduler.engine.kernel.order;

import javax.annotation.Nullable;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.eventbus.EventSource;
import com.sos.scheduler.engine.kernel.variable.UnmodifiableVariableSet;
import com.sos.scheduler.engine.kernel.scheduler.HasPlatform;
import com.sos.scheduler.engine.kernel.order.jobchain.UnmodifiableJobchain;

@ForCpp public interface UnmodifiableOrder extends EventSource, HasPlatform {
	OrderId getId();
	OrderState getState();
	OrderState getEndState();
	String getTitle();
    UnmodifiableJobchain getJobChain();
    @Nullable UnmodifiableJobchain getJobChainOrNull();
	UnmodifiableVariableSet getParameters();
}