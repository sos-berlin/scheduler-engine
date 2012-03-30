package com.sos.scheduler.engine.kernel.order;

public abstract class ModifiableOrderEvent extends OrderEvent {
    protected ModifiableOrderEvent(OrderKey key) {
        super(key);
    }
}
