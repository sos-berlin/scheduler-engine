package com.sos.scheduler.engine.kernel.order.jobchain

import com.google.inject.Injector
import com.sos.scheduler.engine.kernel.cppproxy.Order_queue_nodeCI
import com.sos.scheduler.engine.kernel.order.OrderQueue

abstract class OrderQueueNode extends Node {

  protected val cppProxy: Order_queue_nodeCI
  protected def injector: Injector

  final def orderCount: Int =
    cppProxy.order_queue.java_order_count()

  final def orderQueue: OrderQueue =
    cppProxy.order_queue.getSister
}
