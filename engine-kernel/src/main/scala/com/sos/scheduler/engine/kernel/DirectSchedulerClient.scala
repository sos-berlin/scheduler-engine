package com.sos.scheduler.engine.kernel

import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.folder.FolderTree
import com.sos.scheduler.engine.data.jobchain.{JobChainOverview, JobChainPath}
import com.sos.scheduler.engine.data.order.{OrderOverview, OrderProcessingState}
import com.sos.scheduler.engine.data.queries.{JobChainQuery, OrderQuery}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.job.TaskSubsystem
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.kernel.processclass.ProcessClassSubsystem
import javax.inject.{Inject, Singleton}
import scala.collection.immutable
import scala.concurrent.{ExecutionContext, Future}

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

  def orderOverviewsBy(query: OrderQuery): Future[immutable.Seq[OrderOverview]] =
    directOrSchedulerThreadFuture {
      orderSubsystem.orderOverviews(query)
    }

  def orderTreeComplementedBy(query: OrderQuery) =
    for (o ← ordersComplementedBy(query)) yield
      OrderTreeComplemented(FolderTree.fromHasPaths(query.jobChainPathQuery.folderPath, o.orders), o.usedTasks, o.usedJobs, o.usedProcessClasses)

  def ordersComplementedBy(query: OrderQuery) =
    directOrSchedulerThreadFuture {
      val orderOverviews = orderSubsystem.orderOverviews(query)
      val tasks = orderOverviews map { _.processingState } collect {
        case inTask: OrderProcessingState.InTask ⇒ taskSubsystem.task(inTask.taskId)
      }
      val jobs = (tasks map { _.job }).distinct
      val processClassPaths = (tasks map { _.processClassPath }) ++ (jobs flatMap { _.defaultProcessClassPathOption })
      val processClasses = (processClassPaths map processClassSubsystem.processClass).distinct
      OrdersComplemented(
        orderOverviews,
        (tasks map { _.overview }).sorted,
        (jobs map { _.overview }).sorted,
        (processClasses map { _.overview }).sorted)
    }

  def jobChainOverview(jobChainPath: JobChainPath): Future[JobChainOverview] =
    directOrSchedulerThreadFuture {
      orderSubsystem.jobChain(jobChainPath).overview
    }

  def jobChainOverviewsBy(query: JobChainQuery): Future[immutable.Seq[JobChainOverview]] =
    directOrSchedulerThreadFuture {
      (orderSubsystem.jobChainsByQuery(query) map { _.overview }).toVector
    }

  def jobChainDetails(jobChainPath: JobChainPath) =
    directOrSchedulerThreadFuture {
      orderSubsystem.jobChain(jobChainPath).details
    }
}
