package com.sos.scheduler.engine.kernel.order.jobchain

import com.sos.scheduler.engine.kernel.order.OrderQueue

abstract class OrderQueueNode extends Node {

  private[kernel] def orderCount: Int

  private[kernel] def orderQueue: OrderQueue
}
