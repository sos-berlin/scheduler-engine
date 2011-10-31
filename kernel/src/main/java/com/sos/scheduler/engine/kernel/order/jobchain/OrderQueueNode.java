package com.sos.scheduler.engine.kernel.order.jobchain;

import com.sos.scheduler.engine.kernel.Platform;
import com.sos.scheduler.engine.kernel.cppproxy.Order_queue_nodeC;
import com.sos.scheduler.engine.kernel.job.Job;
import com.sos.scheduler.engine.kernel.order.OrderQueue;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;

public class OrderQueueNode extends JobNode {
    private final Order_queue_nodeC cppProxy;

    private OrderQueueNode(Platform platform, Order_queue_nodeC nodeC) {
        super(platform, nodeC);
        this.cppProxy = nodeC;
    }

    public final Job getJob() {     //TODO Gehört zu JobNode
        return cppProxy.job().getSister();
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
