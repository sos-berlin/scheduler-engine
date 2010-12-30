package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.kernel.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.engine.kernel.cppproxy.Order_queueC;
import com.sos.scheduler.kernel.cplusplus.runtime.Sister;
import com.sos.scheduler.kernel.cplusplus.runtime.SisterType;

/**
 *
 * @author Zschimmer.sos
 */
public class OrderQueue extends AbstractHasPlatform implements Sister {  // Iterable<Order>
    private Order_queueC order_queueC;


    public OrderQueue(Platform platform, Order_queueC a) {
        super(platform);
        this.order_queueC = a;
    }


    @Override public void onCppProxyInvalidated() {}

    
    public int size()  { return order_queueC.java_order_count(); }


    public static class Type implements SisterType<OrderQueue, Order_queueC> {
        @Override public OrderQueue sister(Order_queueC proxy, Sister context) { return new OrderQueue(Platform.of(context), proxy); }
    }
}
