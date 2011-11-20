package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.kernel.scheduler.AbstractHasPlatform;
import com.sos.scheduler.engine.kernel.scheduler.Platform;
import com.sos.scheduler.engine.kernel.cppproxy.Order_queueC;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;

public class OrderQueue extends AbstractHasPlatform implements Sister {  // Iterable<Order>
    private final Order_queueC cppProxy;


    public OrderQueue(Platform platform, Order_queueC cppProxy) {
        super(platform);
        this.cppProxy = cppProxy;
    }


    @Override public final void onCppProxyInvalidated() {}

    
    public final int size()  {
        return cppProxy.java_order_count();
    }


    public static class Type implements SisterType<OrderQueue, Order_queueC> {
        @Override public final OrderQueue sister(Order_queueC proxy, Sister context) { 
            return new OrderQueue(Platform.of(context), proxy);
        }
    }
}
