package com.sos.scheduler.engine.data.order;

import com.sos.scheduler.engine.data.event.ModifiableSourceEvent;

/**
 * This event is fired if a step of an order ended.
 *
 * <i>see also Task::postprocess_order in spooler_task.cxx</i>
 */
public class OrderStepEndedEvent extends OrderEvent implements ModifiableSourceEvent {
    private final OrderStateTransition stateTransistion;

    public OrderStepEndedEvent(OrderKey key, OrderStateTransition stateTransistion) {
        super(key);
        this.stateTransistion = stateTransistion;
    }

    public final OrderStateTransition stateTransition() {
        return stateTransistion;
    }

    @Override public boolean equals(Object o) {
        return o == this || o instanceof OrderStepEndedEvent && eqOrderStepEndedEvent((OrderStepEndedEvent) o);
    }

    private boolean eqOrderStepEndedEvent(OrderStepEndedEvent o) {
        return stateTransistion == o.stateTransistion && super.equals(o);
    }

    @Override public final String toString() {
        return super.toString() +" stateTransition=" + stateTransistion;
    }

    public static OrderStepEndedEvent of(String jobChainPath, String orderId, OrderStateTransition stateTransistion) {
        return new OrderStepEndedEvent(OrderKey.of(jobChainPath, orderId), stateTransistion);
    }
}
