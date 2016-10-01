package com.sos.scheduler.engine.kernel.order.jobchain

import com.sos.scheduler.engine.kernel.cppproxy.Order_queue_nodeCI
import com.sos.scheduler.engine.kernel.order.OrderQueue

abstract class OrderQueueNode extends Node {

  protected[kernel] val cppProxy: Order_queue_nodeCI

  private[kernel] def orderCount: Int

  private[kernel] def orderQueue: OrderQueue

  private[order] def addNonDistributedToOrderStatistics(statisticsArray: Array[Int]): Unit
}
