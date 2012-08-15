package com.sos.scheduler.engine.data.order;

import org.codehaus.jackson.annotate.JsonCreator;
import org.codehaus.jackson.annotate.JsonProperty;

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
