package com.sos.scheduler.engine.kernel.order.jobchain;

import com.sos.scheduler.engine.kernel.order.OrderQueue;
import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.engine.kernel.cppproxy.Order_queue_nodeC;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;


public class OrderQueueNode extends Node {
    private final Order_queue_nodeC order_queue_nodeC;


    public OrderQueueNode(Platform platform, Order_queue_nodeC nodeC) {
        super(platform, nodeC);
        this.order_queue_nodeC = nodeC;
    }


    public OrderQueue orderQueue() { return order_queue_nodeC.order_queue().getSister(); }


    public static class Type implements SisterType<OrderQueueNode, Order_queue_nodeC> {
        @Override public OrderQueueNode sister(Order_queue_nodeC proxy, Sister context) { return new OrderQueueNode(Platform.of(context), proxy); }
    }
}
