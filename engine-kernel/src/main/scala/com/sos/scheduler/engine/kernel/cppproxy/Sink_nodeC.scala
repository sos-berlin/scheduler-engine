package com.sos.scheduler.engine.kernel.cppproxy

import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass
import com.sos.scheduler.engine.kernel.order.jobchain.SinkNode

@CppClass(clas = "sos::scheduler::order::job_chain::Sink_node", directory = "scheduler", include = "spooler.h")
trait Sink_nodeC extends Order_queue_nodeCI with CppProxyWithSister[SinkNode] {

  def order_queue: Order_queueC

  def job_path: String

  def file_order_sink_remove: Boolean

  def file_order_sink_move_to: String
}

object Sink_nodeC {
  def sisterType = SinkNode.Type
}
