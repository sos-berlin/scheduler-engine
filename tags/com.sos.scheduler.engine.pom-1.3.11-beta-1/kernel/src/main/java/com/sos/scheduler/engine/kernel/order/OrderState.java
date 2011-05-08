package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.kernel.util.StringValue;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;


@ForCpp
public class OrderState extends StringValue {
    public OrderState(String a) { super(a); }
    

    public static OrderState of(String a) { return new OrderState(a); }
}
