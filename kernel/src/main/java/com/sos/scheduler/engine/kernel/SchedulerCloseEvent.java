package com.sos.scheduler.engine.kernel;

import com.sos.scheduler.engine.kernel.event.ObjectEvent;

public class SchedulerCloseEvent extends ObjectEvent {
	private final Scheduler scheduler;
	
    public SchedulerCloseEvent(Scheduler o) {
        scheduler = o;
    }

	protected SchedulerObject getObject() {
		return scheduler;
	}
}
