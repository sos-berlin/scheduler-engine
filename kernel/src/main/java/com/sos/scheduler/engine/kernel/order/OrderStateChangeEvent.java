package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;


@ForCpp
public class OrderStateChangeEvent extends OrderEvent {
    private final OrderState previousState;

    
    public OrderStateChangeEvent(Order order, OrderState previousState) {
        super(order);
        this.previousState = previousState;
    }


    public OrderState getPreviousState() { return previousState; }


    @Override public String toString() { return super.toString() + ", previousState=" + previousState; }
}
