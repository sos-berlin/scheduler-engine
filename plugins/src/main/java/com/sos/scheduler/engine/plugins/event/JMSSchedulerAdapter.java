package com.sos.scheduler.engine.plugins.event;

import com.sos.scheduler.engine.kernel.Scheduler;
import com.sos.scheduler.model.events.InfoScheduler;

public class JMSSchedulerAdapter extends InfoScheduler {
	
	private JMSSchedulerAdapter(Scheduler kernelEvent) {
		super();
		setHostname(kernelEvent.getHostname());
		setPort(kernelEvent.getTcpPort());
		setHttpUrl(kernelEvent.getHttpUrl());
		setVersion(kernelEvent.getVersion());
		setId(kernelEvent.getSchedulerId());
	}

	public static JMSSchedulerAdapter createInstance(Scheduler kernelEvent) {
		return new JMSSchedulerAdapter(kernelEvent);
	}
}
