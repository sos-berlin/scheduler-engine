package com.sos.scheduler.engine.kernel.order;

/**
 * \file OrderFinishedEvent.java
 * \brief This event fired if an order reach the end state 
 *  
 * \class OrderFinishedEvent
 * \brief This event fired if an order reach the end state 
 * 
 * \details
 * 
 * \see
 * Order::handle_end_state in Order.cxx
 *
 * \code
  \endcode
 *
 * \version 1.0 - 12.04.2011 12:07:55
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */
public class OrderFinishedEvent extends OrderEvent {
    public OrderFinishedEvent(OrderKey key) {
        super(key);
    }
}
