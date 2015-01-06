package com.sos.scheduler.engine.kernel.cppproxy;

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy;
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass;
import com.sos.scheduler.engine.kernel.order.Order;

@CppClass(clas="sos::scheduler::order::Standing_order_subsystem", directory="scheduler", include="spooler.h")
public interface Standing_order_subsystemC extends SubsystemC<Order, OrderC>, CppProxy {}
