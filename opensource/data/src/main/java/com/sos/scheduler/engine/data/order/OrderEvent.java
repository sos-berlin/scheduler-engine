package com.sos.scheduler.engine.data.order;

import com.sos.scheduler.engine.data.event.AbstractEvent;

import static com.google.common.base.Objects.equal;

public abstract class OrderEvent extends AbstractEvent {
    private final OrderKey key;

    protected OrderEvent(OrderKey key) {
        this.key = key;
    }

    public final OrderKey getKey() {
        return key;
    }

    @Override public boolean equals(Object o) {
        return o == this || o instanceof OrderEvent && eq((OrderEvent)o);
    }

    private boolean eq(OrderEvent o) {
        return equal(key, o.key);
    }

    @Override public String toString() {
        return super.toString() +" "+ key;
    }
}