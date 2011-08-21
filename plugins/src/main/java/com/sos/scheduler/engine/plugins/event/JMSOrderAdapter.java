package com.sos.scheduler.engine.plugins.event;

import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.model.events.Order;;

public class JMSOrderAdapter extends Order {
	
	private JMSOrderAdapter(UnmodifiableOrder kernelEventOrder) {
		super();
		setId(kernelEventOrder.getId().getString());
		setTitle(kernelEventOrder.getTitle());
		setState(kernelEventOrder.getState().toString());
	}

	public static JMSOrderAdapter createInstance(UnmodifiableOrder kernelEventOrder) {
		return new JMSOrderAdapter(kernelEventOrder);
	}
}
