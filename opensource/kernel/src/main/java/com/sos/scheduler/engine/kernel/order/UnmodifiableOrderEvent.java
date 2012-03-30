package com.sos.scheduler.engine.kernel.order;

abstract class UnmodifiableOrderEvent extends OrderEvent {
    protected UnmodifiableOrderEvent(OrderKey key) {
        super(key);
    }
}