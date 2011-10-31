package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.kernel.order.jobchain.Node;
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import java.util.List;

@CppClass(clas="sos::scheduler::order::Job_chain", directory="scheduler", include="spooler.h")
public interface Job_chainC extends CppProxyWithSister<JobChain> {
    JobChain.Type sisterType = new JobChain.Type();
    String name();
    void set_force_file_reload();
    List<Node> java_nodes();
    OrderC order(String orderId);
}
