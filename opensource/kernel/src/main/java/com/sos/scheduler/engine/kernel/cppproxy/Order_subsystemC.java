package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;

@CppClass(clas="sos::scheduler::order::Order_subsystem", directory="scheduler", include="spooler.h")
public interface Order_subsystemC extends CppProxy, SubsystemC<JobChain, Job_chainC>  {

    int finished_orders_count();
}
