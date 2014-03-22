package com.sos.scheduler.engine.kernel.cppproxy;

import java.util.List;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.Sister;
import com.sos.scheduler.engine.cplusplus.runtime.SisterType;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.kernel.order.jobchain.Node;
import com.sos.scheduler.engine.kernel.scheduler.HasInjector;

@CppClass(clas="sos::scheduler::order::Job_chain", directory="scheduler", include="spooler.h")
public interface Job_chainC extends CppProxyWithSister<JobChain>, File_basedC<JobChain> {
    SisterType<JobChain, Job_chainC> sisterType = new SisterType<JobChain, Job_chainC>() {
        public JobChain sister(Job_chainC proxy, Sister context) {
            return new JobChain(proxy, ((HasInjector)context).injector());
        }
    };

    List<Node> java_nodes();
    OrderC order(String orderId);
    OrderC order_or_null(String orderID);
    boolean is_stopped();
    void set_stopped(boolean o);
    void remove();
}
