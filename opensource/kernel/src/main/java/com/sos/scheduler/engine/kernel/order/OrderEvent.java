package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.kernel.scheduler.SchedulerObject;
import com.sos.scheduler.engine.kernel.event.ObjectEvent;

public abstract class OrderEvent extends ObjectEvent {
    private final OrderKey key;

    public OrderEvent(OrderKey key) {
        this.key = key;
    }

    @Deprecated
    @Override public final SchedulerObject getObject() {
        return getOrder();
    }

    @Deprecated
    public abstract UnmodifiableOrder getOrder();

    public OrderKey getKey() {
        return key;
    }
}