package com.sos.scheduler.engine.kernel.cppproxy

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain

@CppClass(clas = "sos::scheduler::order::Job_chain", directory = "scheduler", include = "spooler.h")
trait Job_chainC extends CppProxyWithSister[JobChain] with File_basedC[JobChain] {
  def java_nodes: Array[AnyRef]
  def order(orderId: String): OrderC
  def order_or_null(orderID: String): OrderC
  def java_orders: Array[AnyRef]
  def is_stopped: Boolean
  def set_stopped(o: Boolean): Unit
  def max_orders: Int
  def remove(): Unit
  def is_distributed: Boolean
  def default_process_class_path: String
  def file_watching_process_class_path: String
  def add_non_distributed_to_order_statistics(counters: Array[Int]): Unit
}

object Job_chainC {
  val sisterType = JobChain.Type
}