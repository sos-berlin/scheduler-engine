package com.sos.scheduler.engine.kernel.order.jobchain;

import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.kernel.scheduler.Platform;
import com.sos.scheduler.engine.kernel.cppproxy.Order_queue_nodeC;
import com.sos.scheduler.engine.kernel.cppproxy.Order_queue_nodeCI;
import com.sos.scheduler.engine.kernel.order.OrderQueue;

public class OrderQueueNode extends Node {
    private final Order_queue_nodeCI cppProxy;

    protected OrderQueueNode(Platform platform, Order_queue_nodeCI nodeC) {
        super(platform, nodeC);
        this.cppProxy = nodeC;
    }

    public final OrderQueue getOrderQueue() {
        return cppProxy.order_queue().getSister();
    }

    public static class Type implements SisterType<OrderQueueNode, Order_queue_nodeC> {
        @Override public final OrderQueueNode sister(Order_queue_nodeC proxy, Sister context) { 
            return new OrderQueueNode(Platform.of(context), proxy);
        }
    }
}
