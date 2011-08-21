package com.sos.scheduler.engine.kernel.order.jobchain;

import com.google.common.collect.ImmutableList;
import com.sos.scheduler.engine.kernel.order.OrderId;
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;

public interface UnmodifiableJobchain {
	String getName();
	ImmutableList<Node> getNodes();
	UnmodifiableOrder getUnmodifiableOrder(OrderId id);
}