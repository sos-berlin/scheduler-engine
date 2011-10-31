package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.order.jobchain.JobNode;

@CppClass(clas="sos::scheduler::order::job_chain::Job_node", directory="scheduler", include="spooler.h")
public interface Job_nodeC extends Order_queue_nodeCI, CppProxyWithSister<JobNode>  {
    JobNode.Type sisterType = new JobNode.Type();

    JobC job();
}
