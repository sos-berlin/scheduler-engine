package com.sos.scheduler.engine.plugins.event;

import com.sos.scheduler.model.events.InfoLog;

public class JMSMessageAdapter extends InfoLog {
	
	private JMSMessageAdapter(com.sos.scheduler.engine.kernel.event.Message eventMessage) {
		super();
		setCode(eventMessage.getCode());
		setMessage(eventMessage.toString());
	}

	public static JMSMessageAdapter createInstance(com.sos.scheduler.engine.kernel.event.Message eventMessage) {
		return new JMSMessageAdapter(eventMessage);
	}
}
