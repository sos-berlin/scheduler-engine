package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.order.jobchain.SimpleJobNode;

@CppClass(clas="sos::scheduler::order::job_chain::Job_node", directory="scheduler", include="spooler.h")
public interface Job_nodeC extends Order_queue_nodeCI, CppProxyWithSister<SimpleJobNode>  {
    SimpleJobNode.Type sisterType = new SimpleJobNode.Type();

    JobC job();

    String job_path();

    String[] missing_requisites_java();
}
