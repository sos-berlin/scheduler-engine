package com.sos.scheduler.engine.kernel.cppproxy

trait Order_queue_nodeCI extends NodeCI {

  def order_queue: Order_queueC

  def add_non_distributed_to_order_statistics(counters: Array[Int]): Unit
}
