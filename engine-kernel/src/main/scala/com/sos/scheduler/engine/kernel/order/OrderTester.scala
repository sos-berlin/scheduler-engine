package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.common.time.Stopwatch
import com.sos.scheduler.engine.data.order.OrderQuery
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import javax.inject.{Inject, Singleton}

/**
  * @author Joacim Zschimmer
  */
@Singleton
private[engine] final class OrderTester @Inject private(orderSubsystem: OrderSubsystem)
  (implicit schedulerThreadCallQueue: SchedulerThreadCallQueue) {

  def testSpeed(m: Int, n: Int): Unit = {
    inSchedulerThread {
      val order = orderSubsystem.ordersByQuery(OrderQuery.All).next()
      val orderC = order.cppProxy
      val m = 3
      val n = 10000
      for (_ ← 1 to m) Stopwatch.measureTime(n, "java_fast_flags") {
        orderC.java_fast_flags
      }
      for (_ ← 1 to m) Stopwatch.measureTime(n, "is_file_order") {
        orderC.is_file_order
      }
      for (_ ← 1 to m) Stopwatch.measureTime(n, "is_file_based") {
        orderC.is_file_based
      }
      for (_ ← 1 to m) Stopwatch.measureTime(n, "file_based_state") {
        orderC.file_based_state
      }
      for (_ ← 1 to m) Stopwatch.measureTime(n, "suspended") {
        orderC.suspended
      }
      for (_ ← 1 to m) Stopwatch.measureTime(n, "is_on_blacklist") {
        orderC.is_on_blacklist
      }
      for (_ ← 1 to m) Stopwatch.measureTime(n, "java_job_chain_node") {
        orderC.java_job_chain_node
      }
      for (_ ← 1 to m) Stopwatch.measureTime(n, "job_chain") {
        orderC.job_chain
      }
      for (_ ← 1 to m) Stopwatch.measureTime(n, "task_id") {
        orderC.task_id
      }
      for (_ ← 1 to m) Stopwatch.measureTime(n, "OrderC") {
        orderC.java_fast_flags
        orderC.job_chain
        orderC.java_job_chain_node.orderState
        orderC.task_id
      }
      for (_ ← 1 to m) Stopwatch.measureTime(n, "sourceType") {
        order.sourceType
      }
      for (_ ← 1 to m) Stopwatch.measureTime(n, "sameOrder.overview") {
        order.overview
      }
      for (_ ← 1 to m) Stopwatch.measureTime(n, "Order") {
        orderSubsystem.ordersByQuery(OrderQuery.All).toVector
      }
      val orders = orderSubsystem.ordersByQuery(OrderQuery.All).toVector
      for (_ ← 1 to m) Stopwatch.measureTime(n, "foreach-Order.overview") {
        for (o <- orders) o.overview
      }
      for (_ ← 1 to m) Stopwatch.measureTime(n, "foreach-Order.sourceType") {
        for (o <- orders) o.sourceType
      }
      for (_ ← 1 to m) Stopwatch.measureTime(n, "foreach-Order.isSuspended") {
        for (o <- orders) o.isSuspended
      }
      for (_ ← 1 to m) Stopwatch.measureTime(n, "foreach-Order.java_fast_flags") {
        for (o <- orders) o.cppProxy.java_fast_flags
      }
    }
  }
}
