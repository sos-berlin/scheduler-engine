package com.sos.scheduler.engine.kernel.cppproxy

import com.sos.scheduler.engine.kernel.order.{OrderCallback, OrderQueue}
import com.sos.scheduler.engine.cplusplus.runtime.CppProxyWithSister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.CppClass


@CppClass(clas = "sos::scheduler::order::Order_queue", directory = "scheduler", include = "spooler.h")
trait Order_queueC extends CppProxyWithSister[OrderQueue] {

  def close(): Unit

  def java_order_count: Int

  def is_distributed_order_requested(now: Long): Boolean
}

object Order_queueC {
  val sisterType = OrderQueue.Type
}
