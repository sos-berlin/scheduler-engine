package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.kernel.cplusplus.runtime.annotation.ForCpp;


@ForCpp
public class OrderTouchedEvent extends OrderEvent {
    public OrderTouchedEvent(Order order) {
        super(order);
    }
}
