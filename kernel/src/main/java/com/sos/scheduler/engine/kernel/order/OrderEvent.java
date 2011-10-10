package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.kernel.SchedulerObject;
import com.sos.scheduler.engine.kernel.event.ObjectEvent;

public abstract class OrderEvent extends ObjectEvent {
    @Override protected final SchedulerObject getObject() {
        return getOrder();
    }

    public abstract UnmodifiableOrder getOrder();
}