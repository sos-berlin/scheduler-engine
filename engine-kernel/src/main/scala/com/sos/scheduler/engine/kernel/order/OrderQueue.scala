package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.kernel.cppproxy.Order_queueC

final class OrderQueue(cppProxy: Order_queueC) extends Sister {
  def onCppProxyInvalidated() {}

  def size = cppProxy.java_order_count
}

object OrderQueue {

  object Type extends SisterType[OrderQueue, Order_queueC] {
    final def sister(proxy: Order_queueC, context: Sister): OrderQueue = new OrderQueue(proxy)
  }
}
