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
    private final boolean ok;

    public OrderStepEndedEvent(OrderKey key, boolean ok) {
        super(key);
        this.ok = ok;
    }

    public final boolean isOk() {
        return ok;
    }

    @Override public boolean equals(Object o) {
        return o == this || o instanceof OrderStepEndedEvent && eqOrderStepEndedEvent((OrderStepEndedEvent) o);
    }

    private boolean eqOrderStepEndedEvent(OrderStepEndedEvent o) {
        return ok == o.ok && super.equals(o);
    }

    @Override public final String toString() {
        return super.toString() + ", ok=" + ok;
    }

    public static OrderStepEndedEvent of(String jobChainPath, String orderId, boolean ok) {
        return new OrderStepEndedEvent(OrderKey.of(jobChainPath, orderId), ok);
    }
}
