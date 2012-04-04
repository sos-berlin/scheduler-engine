package com.sos.scheduler.engine.kernel.order;

import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.kernel.cppproxy.Order_queueC;

public class OrderQueue implements Sister {  // Iterable<Order>
    private final Order_queueC cppProxy;

    public OrderQueue(Order_queueC cppProxy) {
        this.cppProxy = cppProxy;
    }

    @Override public final void onCppProxyInvalidated() {}

    public final int size()  {
        return cppProxy.java_order_count();
    }

    public static class Type implements SisterType<OrderQueue, Order_queueC> {
        @Override public final OrderQueue sister(Order_queueC proxy, Sister context) { 
            return new OrderQueue(proxy);
        }
    }
}
