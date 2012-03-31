package com.sos.scheduler.engine.data.order;

import com.sos.scheduler.engine.data.base.StringValue;

public class OrderState extends StringValue {
    public OrderState(String a) {
        super(a);
    }
    
    public static OrderState of(String a) { return new OrderState(a); }
}
