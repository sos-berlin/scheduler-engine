package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.kernel.order.Order;
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;

@CppClass(clas="sos::scheduler::order::Order", directory="scheduler", include="spooler.h")
public interface OrderC extends CppProxyWithSister<Order> {
    Order.Type sisterType = new Order.Type();

    String path();
    String file_path();
    String job_chain_path_string();
    Job_chainC job_chain();
    String string_id();
    void set_id(String id);
    String title();
    Variable_setC params();
    //    Node job_chain_node();
    String string_state();
    void set_end_state(String order_state);
    String string_end_state();
    Prefix_logC log();
}
