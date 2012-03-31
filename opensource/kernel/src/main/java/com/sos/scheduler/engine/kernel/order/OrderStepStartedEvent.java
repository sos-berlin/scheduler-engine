package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.eventbus.ModifiableSourceEvent;

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
@ForCpp
public class OrderStepStartedEvent extends OrderEvent implements ModifiableSourceEvent {
    public OrderStepStartedEvent(OrderKey key) {
        super(key);
    }
}
