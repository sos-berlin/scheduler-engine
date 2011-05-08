package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.kernel.order.jobchain.Node;
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;


@CppClass(clas="sos::scheduler::order::job_chain::Node", directory="scheduler", include="spooler.h")
public interface NodeC extends NodeCI, CppProxyWithSister<Node>
{
    Node.Type sisterType = new Node.Type();
}
