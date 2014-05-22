package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.order.jobchain.NestedJobChainNode;

@CppClass(clas="sos::scheduler::order::job_chain::Nested_job_chain_node", directory="scheduler", include="spooler.h")
public interface Nested_job_chain_nodeC extends NodeCI, CppProxyWithSister<NestedJobChainNode> {
    NestedJobChainNode.Type sisterType = new NestedJobChainNode.Type();

    String nested_job_chain_path();
}
