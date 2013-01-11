package com.sos.scheduler.engine.data.order;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.sos.scheduler.engine.data.event.ModifiableSourceEvent;

/**
 * This event fired if an order scheduled by the JS
 *  
 * <i>see also Order::occupy_for_task in Order.cxx</i>
 */
public class OrderTouchedEvent extends OrderEvent implements ModifiableSourceEvent {
    @JsonCreator
    public OrderTouchedEvent(@JsonProperty("key") OrderKey key) {
        super(key);
    }
}
