package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp;
import com.sos.scheduler.engine.eventbus.ModifiableSourceEvent;

/**
 * \file OrderTouchedEvent.java
 * \brief This event fired if an order scheduled by the JS 
 *  
 * \class OrderTouchedEvent
 * \brief This event fired if an order scheduled by the JS 
 * 
 * \details
 * 
 * \see
 * Order::occupy_for_task in Order.cxx
 *
 * \code
  \endcode
 *
 * \version 1.0 - 12.04.2011 12:00:25
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
@ForCpp
public class OrderTouchedEvent extends OrderEvent implements ModifiableSourceEvent {
    @ForCpp public OrderTouchedEvent(OrderKey key) {
        super(key);
    }
}
