package com.sos.scheduler.engine.kernel

import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented, SchedulerResponse}
import com.sos.scheduler.engine.data.event.{EventId, IdAndEvent}
import com.sos.scheduler.engine.data.events.EventJsonFormat
import com.sos.scheduler.engine.data.folder.FolderTree
import com.sos.scheduler.engine.data.job.{JobOverview, JobPath, ProcessClassOverview, TaskId, TaskOverview}
import com.sos.scheduler.engine.data.jobchain.{JobChainDetails, JobChainOverview, JobChainPath}
import com.sos.scheduler.engine.data.order.{OrderOverview, OrderProcessingState}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.queries.{JobChainQuery, OrderQuery}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.event.EventCollector
import com.sos.scheduler.engine.kernel.job.{JobSubsystem, TaskSubsystem}
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.kernel.order.jobchain.JobNode
import com.sos.scheduler.engine.kernel.processclass.ProcessClassSubsystem
import javax.inject.{Inject, Singleton}
import scala.collection.immutable
import scala.collection.immutable.Seq
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
  protected val executionContext: ExecutionContext,
  eventCollector: EventCollector)
extends SchedulerClient with DirectCommandClient {

  def overview: Future[SchedulerResponse[SchedulerOverview]] =
    respondWith { scheduler.overview }

  def orderOverviewsBy(query: OrderQuery): Future[SchedulerResponse[immutable.Seq[OrderOverview]]] =
    respondWith { orderSubsystem.orderOverviews(query) }

  def orderTreeComplementedBy(query: OrderQuery) =
    for (schedulerResponse ← ordersComplementedBy(query))
      yield for (o ← schedulerResponse)
        yield OrderTreeComplemented(
          FolderTree.fromHasPaths(query.jobChainPathQuery.folderPath, o.orders),
          o.usedNodes,
          o.usedJobs,
          o.usedTasks,
          o.usedProcessClasses)

  def ordersComplementedBy(query: OrderQuery) =
    respondWith {
      val orderOverviews = orderSubsystem.orderOverviews(query)
      val nodeOverviews = {
        val jobChainPathToNodeKeys = (orderOverviews map { _.nodeKey }).distinct groupBy { _.jobChainPath }
        for ((jobChainPath, nodeKeys) ← jobChainPathToNodeKeys.toVector.sortBy { _._1 };
             jobChain ← orderSubsystem.jobChainOption(jobChainPath).iterator;
             nodeKey ← nodeKeys;
             node ← (jobChain.nodeMap.get(nodeKey.nodeId) collect { case n: JobNode ⇒ n.overview }).toArray sortBy { _.nodeKey.nodeId })  // sort just for determinism - not the original node order
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

  def jobChainOverview(jobChainPath: JobChainPath): Future[SchedulerResponse[JobChainOverview]] =
    respondWith {
      orderSubsystem.jobChain(jobChainPath).overview
    }

  def jobChainOverviewsBy(query: JobChainQuery): Future[SchedulerResponse[Seq[JobChainOverview]]] =
    respondWith {
      (orderSubsystem.jobChainsByQuery(query) map { _.overview }).toVector
    }

  def jobChainDetails(jobChainPath: JobChainPath): Future[SchedulerResponse[JobChainDetails]] =
    respondWith {
      orderSubsystem.jobChain(jobChainPath).details
    }

  def jobOverviews: Future[SchedulerResponse[Vector[JobOverview]]] =
    respondWith {
      jobSubsystem.fileBaseds map { _.overview }
    }

  def jobOverview(jobPath: JobPath): Future[SchedulerResponse[JobOverview]] =
    respondWith {
      jobSubsystem.job(jobPath).overview
    }

  def processClassOverviews: Future[SchedulerResponse[Vector[ProcessClassOverview]]] =
    respondWith {
      processClassSubsystem.fileBaseds map { _.overview }
    }

  def processClassOverview(processClassPath: ProcessClassPath): Future[SchedulerResponse[ProcessClassOverview]] =
    respondWith {
      processClassSubsystem.processClass(processClassPath).overview
    }

  def taskOverview(taskId: TaskId): Future[SchedulerResponse[TaskOverview]] =
    respondWith {
      taskSubsystem.task(taskId).overview
    }

  def events(after: EventId): Future[SchedulerResponse[Seq[IdAndEvent]]] =
    for (events ← eventCollector.iteratorFuture(after)) yield {
      val eventId = eventCollector.newEventId()  // This EventId is only to give the response a timestamp. To continue the event stream, use the last event's EventId.
      val serializables = events filter IdAndEvent.canSerialize
      SchedulerResponse(serializables.toVector)(eventId)
    }

  private def respondWith[A](content: ⇒ A): Future[SchedulerResponse[A]] =
    directOrSchedulerThreadFuture {
      // We are in control of the scheduler thread. No hot scheduler events may occur now.
      // eventCollector.newEventId returns a good EventId usable for the event web service.
      SchedulerResponse(content)(eventCollector.newEventId())
    }
}
