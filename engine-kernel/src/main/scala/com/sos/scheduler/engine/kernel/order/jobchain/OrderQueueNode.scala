package com.sos.scheduler.engine.kernel.order.jobchain

import com.sos.scheduler.engine.kernel.order.OrderQueue

abstract class OrderQueueNode extends Node {

  def orderCount: Int

  def orderQueue: OrderQueue
}
