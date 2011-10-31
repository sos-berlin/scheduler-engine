package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.kernel.order.jobchain.OrderQueueNode;
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;

@CppClass(clas="sos::scheduler::order::job_chain::Order_queue_node", directory="scheduler", include="spooler.h")
public interface Order_queue_nodeC extends Order_queue_nodeCI, CppProxyWithSister<OrderQueueNode> {
    OrderQueueNode.Type sisterType = new OrderQueueNode.Type();
}
