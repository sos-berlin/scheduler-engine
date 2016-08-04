package com.sos.scheduler.engine.kernel

import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.folder.FolderTree
import com.sos.scheduler.engine.data.job.{JobOverview, JobPath, ProcessClassOverview, TaskId, TaskOverview}
import com.sos.scheduler.engine.data.jobchain.{JobChainOverview, JobChainPath}
import com.sos.scheduler.engine.data.order.{OrderOverview, OrderProcessingState}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.queries.{JobChainQuery, OrderQuery}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.job.{JobSubsystem, TaskSubsystem}
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.kernel.order.jobchain.JobNode
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
  jobSubsystem: JobSubsystem,
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
      OrderTreeComplemented(
        FolderTree.fromHasPaths(query.jobChainPathQuery.folderPath, o.orders),
        o.usedNodes,
        o.usedJobs,
        o.usedTasks,
        o.usedProcessClasses)

  def ordersComplementedBy(query: OrderQuery) =
    directOrSchedulerThreadFuture {
      val orderOverviews = orderSubsystem.orderOverviews(query)
      val nodeOverviews = {
        val jobChainPathToNodeKeys = (orderOverviews map { _.nodeKey }).distinct groupBy { _.jobChainPath }
        for ((jobChainPath, nodeKeys) ← jobChainPathToNodeKeys.toVector.sortBy { _._1 };
             jobChain ← orderSubsystem.jobChainOption(jobChainPath).iterator;
             nodeKey ← nodeKeys;
             node ← (jobChain.nodeMap.get(nodeKey.state) collect { case n: JobNode ⇒ n.overview }).toArray sortBy { _.nodeKey.state })  // sort just for determinism - not the original node order
          yield node
      }
      val jobs = {
        val jobPaths = (nodeOverviews map { _.jobPath }).distinct
        jobPaths flatMap jobSubsystem.fileBasedOption
      }
      val tasks = orderOverviews map { _.processingState } collect {
        case inTask: OrderProcessingState.InTask ⇒ taskSubsystem.task(inTask.taskId)
      }
      val processClasses = {
        val processClassPaths = (tasks map { _.processClassPath }) ++ (jobs flatMap { _.defaultProcessClassPathOption })
        processClassPaths.distinct map processClassSubsystem.processClass
      }
      OrdersComplemented(
        orderOverviews,
        nodeOverviews,
        (jobs map { _.overview }).sorted,
        (tasks map { _.overview }).sorted,
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

  def jobOverviews: Future[immutable.Seq[JobOverview]] =
    directOrSchedulerThreadFuture {
      jobSubsystem.fileBaseds map { _.overview }
    }

  def jobOverview(jobPath: JobPath): Future[JobOverview] =
    directOrSchedulerThreadFuture {
      jobSubsystem.job(jobPath).overview
    }

  def processClassOverviews: Future[immutable.Seq[ProcessClassOverview]] =
    directOrSchedulerThreadFuture {
      processClassSubsystem.fileBaseds map { _.overview }
    }

  def processClassOverview(processClassPath: ProcessClassPath): Future[ProcessClassOverview] =
    directOrSchedulerThreadFuture {
      processClassSubsystem.processClass(processClassPath).overview
    }

  def taskOverview(taskId: TaskId): Future[TaskOverview] =
    directOrSchedulerThreadFuture {
      taskSubsystem.task(taskId).overview
    }
}
