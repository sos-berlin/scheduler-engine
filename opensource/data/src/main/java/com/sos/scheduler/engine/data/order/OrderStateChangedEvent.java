package com.sos.scheduler.engine.data.order;

import org.codehaus.jackson.annotate.JsonCreator;
import org.codehaus.jackson.annotate.JsonProperty;

/**
 * This event fired if the order of a job chain changed the state (next state starts)
 * Beside the order object this event provides the state of the previously executed state.
 * 
 * <i>see also Order::set_state2 in Order.cxx</i>
 */
public class OrderStateChangedEvent extends OrderEvent {
    private final OrderState previousState;

    @JsonCreator
    public OrderStateChangedEvent(@JsonProperty("key") OrderKey key, @JsonProperty("previousState") OrderState previousState) {
        super(key);
        this.previousState = previousState;
    }

    /** the state of the state executed previously */
    public final OrderState getPreviousState() {
        return previousState;
    }

    @Override public final String toString() {
        return super.toString() + ", previousState=" + previousState;
    }

    public static OrderStateChangedEvent of(String jobChainPath, String orderId, String state) {
        return new OrderStateChangedEvent(OrderKey.of(jobChainPath, orderId), new OrderState(state));
    }
}
