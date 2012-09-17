package com.sos.scheduler.engine.data.order;

/**
 * This event fired if an order was suspended
 *
 * <i>see also Order::try_place_in_job_chain in Order.cxx</i>
 * <i>see also Order::set_suspended in Order.cxx</i>
 */
public class OrderSuspendedEvent extends OrderEvent {
    public OrderSuspendedEvent(OrderKey key) {
        super(key);
    }
}
