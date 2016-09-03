package com.sos.scheduler.engine.kernel.cppproxy

import com.sos.scheduler.engine.cplusplus.runtime.CppProxy
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass
import com.sos.scheduler.engine.kernel.order.OrderCallback
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain

@CppClass(clas="sos::scheduler::order::Order_subsystem", directory="scheduler", include="spooler.h")
trait Order_subsystemC extends CppProxy with SubsystemC[JobChain, Job_chainC] {

  def finished_orders_count: Int

  def java_for_each_distributed_order(jobChainPaths: java.util.ArrayList[Object], perNodeLimit: Int, callback: OrderCallback): Unit

  def get_statistics(counters: Array[Int]): Unit
}
