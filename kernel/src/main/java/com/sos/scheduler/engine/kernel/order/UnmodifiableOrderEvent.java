package com.sos.scheduler.engine.kernel.order;

abstract class UnmodifiableOrderEvent extends OrderEvent {
    private final UnmodifiableOrder order;

    protected UnmodifiableOrderEvent(UnmodifiableOrder o) {
        order = o;
    }

    @Override public final UnmodifiableOrder getOrder() {
        return order;
    }
}