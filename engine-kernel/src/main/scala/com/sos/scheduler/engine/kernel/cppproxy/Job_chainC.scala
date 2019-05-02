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
  def state: Int
  def max_orders: Int
  def remove(): Unit
  def is_distributed: Boolean
  def default_process_class_path: String
  def file_watching_process_class_path: String
  def order_id_space_name: String
  def title: String
  def nondistributed_order_count: Int
  def nondistributed_blacklisted_order_count: Int
  def blacklistedOrderIds: Array[String]
  def order_source_count: Int
  def java_file_order_source_directory(i: Int): String
  def java_file_order_source_regex(i: Int): String
  def java_file_order_source_repeat_millis(i: Int): Long
  def java_file_order_source_delay_after_error_millis(i: Int): Long
  def java_file_order_source_alert_when_directory_missing(i: Int): Boolean
  def java_file_order_source_files(i: Int): Array[String]
  def java_file_order_source_files_last_modified(i: Int): Array[Long]
}

object Job_chainC {
  val sisterType = JobChain.Type
}
