package com.sos.scheduler.engine.kernel.order;

import javax.annotation.Nullable;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.data.order.OrderId;
import com.sos.scheduler.engine.data.order.OrderKey;
import com.sos.scheduler.engine.data.order.OrderState;
import com.sos.scheduler.engine.eventbus.EventSource;
import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.kernel.variable.UnmodifiableVariableSet;
import com.sos.scheduler.engine.kernel.order.jobchain.UnmodifiableJobchain;

@ForCpp public interface UnmodifiableOrder extends EventSource {
    OrderKey getKey();
	OrderId getId();
	OrderState getState();
	OrderState getEndState();
	String getTitle();
    UnmodifiableJobchain getJobChain();
    @Nullable UnmodifiableJobchain getJobChainOrNull();
	UnmodifiableVariableSet getParameters();
    PrefixLog getLog();
}