package com.sos.scheduler.engine.kernel.order;

public abstract class ModifiableOrderEvent extends OrderEvent {
    private final Order order;

    protected ModifiableOrderEvent(Order o) {
        this.order = o;
    }

    @Override public final Order getOrder() {
        return order;
    }
}
