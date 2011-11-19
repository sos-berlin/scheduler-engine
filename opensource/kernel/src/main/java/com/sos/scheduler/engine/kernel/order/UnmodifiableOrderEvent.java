package com.sos.scheduler.engine.kernel.order;

abstract class UnmodifiableOrderEvent extends OrderEvent {
    private final UnmodifiableOrder order;

    protected UnmodifiableOrderEvent(UnmodifiableOrder o) {
        order = o instanceof Order? ((Order)o).unmodifiableDelegate() : o;
        assert !(order instanceof Order);
    }

    @Override public final UnmodifiableOrder getOrder() {
        return order;
    }
}