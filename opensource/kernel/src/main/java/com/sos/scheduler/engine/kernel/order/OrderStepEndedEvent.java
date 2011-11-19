package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;

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
@ForCpp
public class OrderStepEndedEvent extends ModifiableOrderEvent {
    private final boolean ok;

    @ForCpp public OrderStepEndedEvent(Order order, boolean ok) {
        super(order);
        this.ok = ok;
    }

    public final boolean getOk() {
        return ok;
    }

    @Override public final String toString() {
        return super.toString() + ", ok=" + ok;
    }
}
