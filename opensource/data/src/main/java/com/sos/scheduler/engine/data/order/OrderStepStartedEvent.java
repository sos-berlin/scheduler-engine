package com.sos.scheduler.engine.data.order;

import com.sos.scheduler.engine.data.event.ModifiableSourceEvent;

/**
 * This event is fired before the execution of an order step begins
 *
 * <i>see also Job_module_task::do_step__start in spooler_task.cxx</i>
 */
public class OrderStepStartedEvent extends OrderEvent implements ModifiableSourceEvent {
    private final OrderState state;

    public OrderStepStartedEvent(OrderKey key, OrderState state) {
        super(key);
        this.state = state;
    }

    public final OrderState getState() {
        return state;
    }
}
