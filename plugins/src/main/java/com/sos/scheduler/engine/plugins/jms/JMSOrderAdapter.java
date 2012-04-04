package com.sos.scheduler.engine.plugins.jms;

import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder;
import com.sos.scheduler.engine.kernel.order.Order;
import com.sos.scheduler.model.events.InfoOrder;;

public class JMSOrderAdapter extends InfoOrder {
	
	private JMSOrderAdapter(UnmodifiableOrder kernelEventOrder) {
		super();
		setId(kernelEventOrder.getId().asString());
		setTitle(kernelEventOrder.getTitle());
		setState(kernelEventOrder.getState().toString());
	}

	private JMSOrderAdapter(Order kernelEventOrder) {
		super();
		setId(kernelEventOrder.getId().asString());
		setTitle(kernelEventOrder.getTitle());
		setState(kernelEventOrder.getState().toString());
	}

	public static JMSOrderAdapter createInstance(UnmodifiableOrder kernelEventOrder) {
		return new JMSOrderAdapter(kernelEventOrder);
	}

	public static JMSOrderAdapter createInstance(Order kernelEventOrder) {
		return new JMSOrderAdapter(kernelEventOrder);
	}
}
