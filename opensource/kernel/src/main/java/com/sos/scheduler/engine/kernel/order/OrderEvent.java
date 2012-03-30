package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.eventbus.AbstractEvent;

public abstract class OrderEvent extends AbstractEvent {
    private final OrderKey key;

    protected OrderEvent(OrderKey key) {
        this.key = key;
    }

    public OrderKey getKey() {
        return key;
    }
}