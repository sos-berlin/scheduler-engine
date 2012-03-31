package com.sos.scheduler.engine.data.order;

import com.sos.scheduler.engine.data.event.ModifiableSourceEvent;

/**
 * \file OrderStepStartedEvent.java
 * \brief This event is fired before the execution of an order step begins 
 *  
 * \class OrderStepStartedEvent
 * \brief This event is fired before the execution of an order step begins 
 * 
 * \details
 * 
 * \see
 * Task::do_something in spooler_task.cxx
 *
 * \code
  \endcode
 *
 * \version 1.0 - 26.08.2011 10:02
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
public class OrderStepStartedEvent extends OrderEvent implements ModifiableSourceEvent {
    public OrderStepStartedEvent(OrderKey key) {
        super(key);
    }
}
