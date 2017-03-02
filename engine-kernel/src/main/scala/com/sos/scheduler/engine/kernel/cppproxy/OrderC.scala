package com.sos.scheduler.engine.kernel.cppproxy

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.{CppClass, CppExpression}
import com.sos.scheduler.engine.kernel.order.Order
import com.sos.scheduler.engine.kernel.order.jobchain.Node

@CppClass(clas="sos::scheduler::order::Order", directory="scheduler", include="spooler.h")
trait OrderC extends CppProxyWithSister[Order] with File_basedC[Order] {
  def file_path: String
  def job_chain_path_string: String
  def job_chain: Job_chainC
  def outer_job_chain_path: String
  def string_id: String
  def set_id(id: String): Unit
  def title: String
  def set_title(o: String): Unit
  def string_payload: String
  def database_runtime_xml: String
  def database_xml: String
  def params: Variable_setC
  //    Node job_chain_node();
  def priority: Int
  def set_priority(o: Int): Unit
  def string_state: String
  def initial_state_string: String
  def set_end_state(order_state: String): Unit
  def end_state_string: String
  def java_remove(): Unit
  def suspended: Boolean
  def set_suspended(o: Boolean): Unit
  def next_time_millis: Long
  def calculate_db_distributed_next_time: String
  def is_on_blacklist: Boolean
  def set_on_blacklist(): Unit
  def set_end_state_reached(): Unit
  def next_step_at_millis: Long
  def setback_millis: Long
  def task_id: Int
  def is_file_order: Boolean
  def id_locked: Boolean
  def java_job_chain_node: Node
  def java_fast_flags: Long
  @CppExpression("$->_java_occupying_cluster_member_id")
  def java_occupying_cluster_member_id: String
  def history_id: Int
  @CppExpression("$->start_time().millis()")
  def startTimeMillis: Long
  @CppExpression("$->end_time().millis()")
  def endTimeMillis: Long
}

object OrderC {
  val sisterType = Order.Type
}
