package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.kernel.scheduler.SchedulerObject;
import com.sos.scheduler.engine.kernel.event.ObjectEvent;

public abstract class OrderEvent extends ObjectEvent {
    @Deprecated
    @Override public final SchedulerObject getObject() {
        return getOrder();
    }

    @Deprecated
    public abstract UnmodifiableOrder getOrder();

    public OrderKey getKey() {
        return new OrderKey(getOrder().getJobChain().getPath(), getOrder().getId());
    }
}