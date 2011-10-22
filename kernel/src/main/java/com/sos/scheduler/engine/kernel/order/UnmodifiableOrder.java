package com.sos.scheduler.engine.kernel.order;

import javax.annotation.Nullable;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.HasPlatform;
import com.sos.scheduler.engine.kernel.UnmodifiableVariableSet;
import com.sos.scheduler.engine.kernel.order.jobchain.UnmodifiableJobchain;

@ForCpp public interface UnmodifiableOrder extends HasPlatform {
	OrderId getId();
	OrderState getState();
	OrderState getEndState();
	String getTitle();
	@Nullable UnmodifiableJobchain jobchainOrNull();
	UnmodifiableVariableSet getParameters();
}