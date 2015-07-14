package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.order.jobchain.SinkNode;

@CppClass(clas="sos::scheduler::order::job_chain::Sink_node", directory="scheduler", include="spooler.h")
public interface Sink_nodeC extends NodeCI, CppProxyWithSister<SinkNode> {
    SinkNode.Type sisterType = new SinkNode.Type();

    Order_queueC order_queue();
    String job_path();
    boolean file_order_sink_remove();
    String file_order_sink_move_to();
}
