package com.sos.scheduler.engine.data.order;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;

/**
 * This event is fired if an order reached the end state.
 *
 * <i>see also Order::handle_end_state in Order.cxx</i>
 */
public class OrderFinishedEvent extends OrderEvent {
    @JsonCreator
    public OrderFinishedEvent(@JsonProperty("key") OrderKey key) {
        super(key);
    }
}
