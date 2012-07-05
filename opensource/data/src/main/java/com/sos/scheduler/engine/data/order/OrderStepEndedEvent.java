package com.sos.scheduler.engine.data.order;

import com.sos.scheduler.engine.data.event.ModifiableSourceEvent;

/**
 * \file OrderStepEndedEvent.java
 * \brief This event is fired if a step of an order ended. 
 *  
 * \class OrderStepEndedEvent
 * \brief This event is fired if a step of an order ended. 
 * 
 * \details
 * 
 * \see
 * Task::step__end in spooler_task.cxx
 *
 * \code
  \endcode
 *
 * \version 1.0 - 26.08.2011 10:05
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
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
