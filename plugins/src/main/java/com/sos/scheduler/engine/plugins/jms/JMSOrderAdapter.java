package com.sos.scheduler.engine.plugins.jms;

import com.sos.scheduler.model.events.Order;;

public class JMSOrderAdapter extends Order {
	
	private JMSOrderAdapter(com.sos.scheduler.engine.kernel.order.Order kernelEventOrder) {
		super();
		setId(kernelEventOrder.getId().getString());
		setTitle(kernelEventOrder.getTitle());
		setState(kernelEventOrder.getState().toString());
	}

	public static JMSOrderAdapter createEventOrder(com.sos.scheduler.engine.kernel.order.Order kernelEventOrder) {
		return new JMSOrderAdapter(kernelEventOrder);
	}
}
