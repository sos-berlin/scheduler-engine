package com.sos.scheduler.engine.kernel.order;

public abstract class UnmodifiableOrderEvent extends OrderEvent {
    private final UnmodifiableOrder order;

    public UnmodifiableOrderEvent(UnmodifiableOrder o) {
        order = o;
    }

    @Override public final UnmodifiableOrder getOrder() {
        return order;
    }
}