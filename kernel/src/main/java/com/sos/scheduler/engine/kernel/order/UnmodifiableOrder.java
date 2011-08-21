package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.kernel.HasPlatform;
import com.sos.scheduler.engine.kernel.UnmodifiableVariableSet;
import com.sos.scheduler.engine.kernel.order.jobchain.UnmodifiableJobchain;

@ForCpp public interface UnmodifiableOrder extends HasPlatform {
	OrderId getId();
	OrderState getState();
	void getFilePath();
	OrderState getEndState();
	String getTitle();
	UnmodifiableJobchain unmodifiableJobchainOrNull();
	UnmodifiableVariableSet getUnmodifiableParameters();
}