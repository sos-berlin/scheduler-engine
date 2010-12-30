package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.kernel.order.OrderQueue;
import com.sos.scheduler.kernel.cplusplus.runtime.CppProxyWithSister;
import com.sos.scheduler.kernel.cplusplus.runtime.annotation.CppClass;


@CppClass(clas="sos::scheduler::order::Order_queue", directory="scheduler", include="spooler.h")
public interface Order_queueC extends CppProxyWithSister<OrderQueue> {
    OrderQueue.Type sisterType = new OrderQueue.Type();

    void close();
//    public native Element dom_element(Document d, Show_what w);
//    public native Order_queue_node order_queue_node();
//    public native sos.spooler.Job_chain job_chain();
//    public native void add_order(sos.spooler.Order o);
    int java_order_count();  //TODO Read_transaction übergeben
    boolean is_distributed_order_requested(long now);
//    String obj_name();
}
