package com.sos.scheduler.intern.cppproxy;

import com.sos.cplusplus.runtime.CppProxy;
import com.sos.cplusplus.runtime.annotation.CppClass;


@CppClass(clas="sos::scheduler::Order_queue", directory="scheduler", include="spooler.h")
public interface Order_queueC extends CppProxy {
    void close();
    String obj_name();
//    public native Element dom_element(Document d, Show_what w);
//    public native Order_queue_node order_queue_node();
//    public native sos.spooler.Job_chain job_chain();
//    public native void add_order(sos.spooler.Order o);
//    public native int order_count(Read_transaction t);
    boolean is_distributed_order_requested(long now);
}
