package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.kernel.order.jobchain.Node;
import com.sos.scheduler.kernel.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.kernel.cplusplus.runtime.annotation.CppClass;
import java.util.List;


@CppClass(clas="sos::scheduler::order::Job_chain", directory="scheduler", include="spooler.h")
public interface Job_chainC extends CppProxyWithSister<JobChain>
{
    JobChain.Type sisterType = new JobChain.Type();
    List<Node> java_nodes();
    OrderC order(String orderId);
    String name();
}
