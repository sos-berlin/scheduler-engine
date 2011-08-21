package com.sos.scheduler.engine.kernel.order;

abstract public class OrderEvent extends GenericOrderEvent<UnmodifiableOrder> {
    public OrderEvent(UnmodifiableOrder o) {
        super(o);
    }
}