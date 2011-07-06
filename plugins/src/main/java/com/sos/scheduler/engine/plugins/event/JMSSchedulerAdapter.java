package com.sos.scheduler.engine.plugins.event;

import com.sos.scheduler.model.events.Scheduler;

public class JMSSchedulerAdapter extends Scheduler {
	
	private JMSSchedulerAdapter(com.sos.scheduler.engine.kernel.Scheduler kernelEvent) {
		super();
		setHostname(kernelEvent.getHostname());
		setPort(kernelEvent.getTcpPort());
		setHttpUrl(kernelEvent.getHttpUrl());
		setVersion(kernelEvent.getVersion());
		setId(kernelEvent.getSchedulerId());
	}

	public static JMSSchedulerAdapter createInstance(com.sos.scheduler.engine.kernel.Scheduler kernelEvent) {
		return new JMSSchedulerAdapter(kernelEvent);
	}
}
