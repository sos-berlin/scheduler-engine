package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.kernel.order.jobchain.OrderQueueNode;
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;


@CppClass(clas="sos::scheduler::order::job_chain::Order_queue_node", directory="scheduler", include="spooler.h")
public interface Order_queue_nodeC extends NodeCI, CppProxyWithSister<OrderQueueNode>
{
    OrderQueueNode.Type sisterType = new OrderQueueNode.Type();

    JobC getJob();
    Order_queueC order_queue();
}
