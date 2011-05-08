package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.kernel.event.ObjectEvent;


public class OrderEvent extends ObjectEvent<Order> {
    public OrderEvent(Order o) {
        super(o);
    }


    /** Für JavaScript, das getObject() nicht sieht. */  //TODO Warum ist das so? 
    public Order getOrder() {
        return getObject();
    }
}
