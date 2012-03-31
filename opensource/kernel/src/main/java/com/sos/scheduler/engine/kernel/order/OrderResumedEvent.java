package com.sos.scheduler.engine.kernel.order;

/**
 * \file OrderResumedEvent.java
 * \brief This event fired if an order was resumed 
 *  
 * \class OrderResumedEvent
 * \brief This event fired if an order was resumed 
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
public class OrderResumedEvent extends OrderEvent {
    public OrderResumedEvent(OrderKey key) {
        super(key);
    }

    public static OrderResumedEvent of(String jobChainPath, String orderId) {
        return new OrderResumedEvent(OrderKey.of(jobChainPath, orderId));
    }
}
