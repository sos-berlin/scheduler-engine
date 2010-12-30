package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.kernel.cplusplus.runtime.annotation.ForCpp;


@ForCpp
public class OrderFinishedEvent extends OrderEvent {
    public OrderFinishedEvent(Order order) {
        super(order);
    }
}
