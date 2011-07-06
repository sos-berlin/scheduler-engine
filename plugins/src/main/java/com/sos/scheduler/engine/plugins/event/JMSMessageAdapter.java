package com.sos.scheduler.engine.plugins.event;

import com.sos.scheduler.model.events.Log;

public class JMSMessageAdapter extends Log {
	
	private JMSMessageAdapter(com.sos.scheduler.engine.kernel.event.Message eventMessage) {
		super();
		setCode(eventMessage.getCode());
		setMessage(eventMessage.toString());
	}

	public static JMSMessageAdapter createInstance(com.sos.scheduler.engine.kernel.event.Message eventMessage) {
		return new JMSMessageAdapter(eventMessage);
	}
}
