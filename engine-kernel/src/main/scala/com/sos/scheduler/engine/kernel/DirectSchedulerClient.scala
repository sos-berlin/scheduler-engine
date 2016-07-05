package com.sos.scheduler.engine.kernel

import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.data.compounds.OrdersFullOverview
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.job.TaskSubsystem
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.kernel.processclass.ProcessClassSubsystem
import javax.inject.{Inject, Singleton}
import scala.concurrent.ExecutionContext

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class DirectSchedulerClient @Inject private(
  protected val scheduler: Scheduler,
  orderSubsystem: OrderSubsystem,
  taskSubsystem: TaskSubsystem,
  processClassSubsystem: ProcessClassSubsystem)(
  implicit schedulerThreadCallQueue: SchedulerThreadCallQueue,
  protected val executionContext: ExecutionContext)
extends SchedulerClient with DirectCommandClient {

  def overview = scheduler.overview

  def orderOverviews =
    schedulerThreadFuture {
      orderSubsystem.orderOverviews
    }

  def ordersFullOverview =
    schedulerThreadFuture {
      val orderOverviews = orderSubsystem.orderOverviews
      val tasks = orderOverviews flatMap { _.taskId } map taskSubsystem.task
      val jobs = (tasks map { _.job }).distinct
      val processClassPaths = (tasks map { _.processClassPath }) ++ (jobs flatMap { _.defaultProcessClassPathOption })
      val processClasses = (processClassPaths map processClassSubsystem.processClass).distinct
      OrdersFullOverview(
        orderOverviews,
        tasks map { _.overview },
        jobs map { _.overview },
        processClasses map { _.overview })
    }
}
