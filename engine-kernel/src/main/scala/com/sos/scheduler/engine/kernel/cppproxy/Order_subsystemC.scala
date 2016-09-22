package com.sos.scheduler.engine.kernel.cppproxy

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass
import com.sos.scheduler.engine.kernel.order.OrderCallback
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain
import javax.annotation.Nullable

@CppClass(clas="sos::scheduler::order::Order_subsystem", directory="scheduler", include="spooler.h")
trait Order_subsystemC extends CppProxy with SubsystemC[JobChain, Job_chainC] {

  def finished_orders_count: Int

  def java_for_each_distributed_order(
    jobChainPaths: java.util.ArrayList[Object],
    @Nullable orderIds: java.util.ArrayList[Object],
    perNodeLimit: Int,
    callback: OrderCallback): Unit

  def add_non_distributed_to_order_statistics(result: Array[Int]): Unit

  def add_distributed_to_order_statistics(sqlClause: String, result: Array[Int]): Unit
}
