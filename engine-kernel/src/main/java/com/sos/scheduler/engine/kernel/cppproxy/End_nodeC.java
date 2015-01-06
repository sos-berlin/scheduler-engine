package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.order.jobchain.EndNode;

@CppClass(clas="sos::scheduler::order::job_chain::End_node", directory="scheduler", include="spooler.h")
public interface End_nodeC extends NodeCI, CppProxyWithSister<EndNode> {
    EndNode.Type sisterType = new EndNode.Type();
}
