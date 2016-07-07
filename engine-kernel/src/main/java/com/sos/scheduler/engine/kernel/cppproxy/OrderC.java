package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.order.Order;

@CppClass(clas="sos::scheduler::order::Order", directory="scheduler", include="spooler.h")
public interface OrderC extends CppProxyWithSister<Order>, File_basedC<Order> {
    Order.Type sisterType = new Order.Type();

    String file_path();
    String job_chain_path_string();
    Job_chainC job_chain();
    String string_id();
    void set_id(String id);
    String title();
    void set_title(String o);
    String string_payload();
    String database_runtime_xml();
    String database_xml();
    Variable_setC params();
    //    Node job_chain_node();
    int priority();
    void set_priority(int o);
    String string_state();
    String initial_state_string();
    void set_end_state(String order_state);
    String end_state_string();
    void java_remove();
    boolean suspended();
    void set_suspended(boolean o);
    long next_time_millis();
    String calculate_db_distributed_next_time();
    boolean is_on_blacklist();
    void set_on_blacklist();
    void set_end_state_reached();
    long next_step_at_millis();
    long setback_millis();
    int task_id();
    boolean is_file_order();
}
