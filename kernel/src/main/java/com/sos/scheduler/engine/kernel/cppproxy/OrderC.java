package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.kernel.order.Order;
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;


@CppClass(clas="sos::scheduler::order::Order", directory="scheduler", include="spooler.h")
public interface OrderC extends CppProxyWithSister<Order>
{
    Order.Type sisterType = new Order.Type();

    Job_chainC job_chain();
//    Node job_chain_node();
    String string_id();
    String file_path();
    void set_id(String id);
    String string_state();
    void set_end_state(String order_state);
    String string_end_state();
    String title();
    Variable_setC params();
}
