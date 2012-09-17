package com.sos.scheduler.engine.data.order;

/**
 * This event fired if an order was resumed
 *
 * <i>see also Order::set_suspended in Order.cxx</i>
 */
public class OrderResumedEvent extends OrderEvent {
    public OrderResumedEvent(OrderKey key) {
        super(key);
    }
}
