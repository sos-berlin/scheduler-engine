package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.kernel.SchedulerObject;
import com.sos.scheduler.engine.kernel.event.ObjectEvent;

public class GenericOrderEvent<ORDER extends UnmodifiableOrder> extends ObjectEvent {
	private final ORDER order;
	
    public GenericOrderEvent(ORDER o) {
        this.order = o;
    }

	public ORDER getOrder() {
		return order;
	}
	
	protected SchedulerObject getObject() {
		return order;
	}
}
