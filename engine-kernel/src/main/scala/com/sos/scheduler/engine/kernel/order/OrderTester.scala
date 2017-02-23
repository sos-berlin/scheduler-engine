package com.sos.scheduler.engine.kernel.order

import com.sos.jobscheduler.common.time.Stopwatch.measureTime
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import javax.inject.{Inject, Singleton}

/**
  * @author Joacim Zschimmer
  */
@Singleton
private[engine] final class OrderTester @Inject private(orderSubsystem: OrderSubsystem)
  (implicit schedulerThreadCallQueue: SchedulerThreadCallQueue) {

  def testSpeed(orderKey: OrderKey, m: Int, n: Int): Unit = {
    val order = inSchedulerThread { orderSubsystem.order(orderKey) }
    val orderC = inSchedulerThread { order.cppProxy }
    val m = 3
    val n = 10000
    inSchedulerThread {
      for (_ ← 1 to m) measureTime(n, "java_fast_flags") {
        orderC.java_fast_flags
      }
    }
    inSchedulerThread {
      for (_ ← 1 to m) measureTime(n, "is_file_order") {
        orderC.is_file_order
      }
    }
    inSchedulerThread {
      for (_ ← 1 to m) measureTime(n, "is_file_based") {
        orderC.is_file_based
      }
    }
    inSchedulerThread {
      for (_ ← 1 to m) measureTime(n, "file_based_state") {
        orderC.file_based_state
      }
    }
    inSchedulerThread {
      for (_ ← 1 to m) measureTime(n, "suspended") {
        orderC.suspended
      }
    }
    inSchedulerThread {
      for (_ ← 1 to m) measureTime(n, "is_on_blacklist") {
        orderC.is_on_blacklist
      }
    }
    inSchedulerThread {
      for (_ ← 1 to m) measureTime(n, "java_job_chain_node") {
        orderC.java_job_chain_node
      }
    }
    inSchedulerThread {
      for (_ ← 1 to m) measureTime(n, "job_chain") {
        orderC.job_chain
      }
    }
    inSchedulerThread {
      for (_ ← 1 to m) measureTime(n, "task_id") {
        orderC.task_id
      }
    }
    inSchedulerThread {
      for (_ ← 1 to m) measureTime(n, "OrderC.{java_fast_flags job_chain java_job_chain_node.nodeId task_id}") {
        orderC.java_fast_flags
        orderC.job_chain
        orderC.java_job_chain_node.nodeId
        orderC.task_id
      }
    }
    inSchedulerThread {
      for (_ ← 1 to m) measureTime(n, "orderSourceType") {
        order.sourceType
      }
    }
    inSchedulerThread {
      for (_ ← 1 to m) measureTime(n, "fileBasedObstacles") {
        order.fileBasedObstacles
      }
    }
    inSchedulerThread {
      val flags = orderC.java_fast_flags
      val nextStepAtOption = order.nextStepAtOption
      for (_ ← 1 to m) measureTime(n, "orderProcessingState") {
        order.processingState(flags, nextStepAtOption)
      }
    }
    inSchedulerThread {
      val flags = orderC.java_fast_flags
      val nextStepAtOption = order.nextStepAtOption
      val processingState = order.processingState(flags, nextStepAtOption)
      for (_ ← 1 to m) measureTime(n, "obstacles") {
        order.obstacles(flags, processingState)
      }
    }
    inSchedulerThread {
      for (_ ← 1 to m) measureTime(n, "missing_requisites_java") {
        orderC.missing_requisites_java
      }
    }
    inSchedulerThread {
      for (_ ← 1 to m) measureTime(n, "sameOrder.overview") {
        order.overview
      }
    }
    inSchedulerThread {
      for (_ ← 1 to m) measureTime(n, "Order") {
        orderSubsystem.localOrderIterator.toVector
      }
    }
    val orders = inSchedulerThread { orderSubsystem.localOrderIterator.toVector }
    inSchedulerThread {
      for (_ ← 1 to m) measureTime(n, s"(${orders.size} Orders).overview") {
        for (o ← orders) o.overview
      }
    }
    inSchedulerThread {
      for (_ ← 1 to m) measureTime(n, s"(${orders.size} Orders).orderSourceType") {
        for (o ← orders) o.sourceType
      }
    }
    inSchedulerThread {
      for (_ ← 1 to m) measureTime(n, s"(${orders.size} Orders).isSuspended") {
        for (o ← orders) o.isSuspended
      }
    }
    inSchedulerThread {
      for (_ ← 1 to m) measureTime(n, s"(${orders.size} Orders).java_fast_flags") {
        for (o ← orders) o.cppProxy.java_fast_flags
      }
    }
    inSchedulerThread {
      for (_ ← 1 to m) measureTime(n, s"(${orders.size} Orders).missing_requisites_java") {
        for (o ← orders) o.cppProxy.missing_requisites_java
      }
    }
  }
}
