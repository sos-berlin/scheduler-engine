package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import java.util.ArrayList;


@CppClass(clas="sos::scheduler::order::Order_subsystem", directory="scheduler", include="spooler.h")
public interface Order_subsystemC extends CppProxy {
    int finished_orders_count();
    ArrayList<JobChain> java_file_baseds();
    JobChain java_file_based_or_null(String p);
    //String obj_name();
}
