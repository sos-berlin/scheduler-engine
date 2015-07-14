package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.kernel.order.jobchain.Node;
import java.util.List;

@CppClass(clas="sos::scheduler::order::Job_chain", directory="scheduler", include="spooler.h")
public interface Job_chainC extends CppProxyWithSister<JobChain>, File_basedC<JobChain> {
    JobChain.Type sisterType = new JobChain.Type();

    List<Node> java_nodes();
    OrderC order(String orderId);
    OrderC order_or_null(String orderID);
    boolean is_stopped();
    void set_stopped(boolean o);
    int max_orders();
    void remove();
    boolean is_distributed();
    String default_process_class_path();
    String file_watching_process_class_path();
}
