package com.sos.scheduler.engine.kernel

import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.data.agent.AgentAddress
import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.event.Snapshot
import com.sos.scheduler.engine.data.filebased.{FileBasedDetailed, TypedPath}
import com.sos.scheduler.engine.data.job.{JobOverview, JobPath, TaskId, TaskOverview}
import com.sos.scheduler.engine.data.jobchain.{JobChainDetailed, JobChainOverview, JobChainPath}
import com.sos.scheduler.engine.data.order.{OrderKey, OrderProcessingState, OrderStatistics, OrderView}
import com.sos.scheduler.engine.data.processclass.{ProcessClassOverview, ProcessClassPath}
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, JobChainQuery, OrderQuery}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.event.DirectEventClient
import com.sos.scheduler.engine.kernel.event.collector.EventCollector
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.kernel.job.{JobSubsystem, TaskSubsystem}
import com.sos.scheduler.engine.kernel.order.jobchain.JobNode
import com.sos.scheduler.engine.kernel.order.{DirectOrderClient, OrderSubsystem}
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
  processClassSubsystem: ProcessClassSubsystem,
  fileBasedSubsystemRegister: FileBasedSubsystem.Register,
  protected val eventCollector: EventCollector)(
  implicit schedulerThreadCallQueue: SchedulerThreadCallQueue,
  protected val executionContext: ExecutionContext)
extends SchedulerClient with DirectCommandClient with DirectEventClient with DirectOrderClient {

  def overview: Future[Snapshot[SchedulerOverview]] =
    respondWith { scheduler.overview }

  def fileBasedDetailed[P <: TypedPath: TypedPath.Companion](path: P): Future[Snapshot[FileBasedDetailed]] =
    respondWith {
      fileBasedSubsystemRegister.fileBased(path).fileBasedDetailed
    }

  def order[V <: OrderView: OrderView.Companion](orderKey: OrderKey) =
    respondWith { orderSubsystem.order(orderKey).view[V] }

  def ordersBy[V <: OrderView: OrderView.Companion](query: OrderQuery): Future[Snapshot[immutable.Seq[V]]] =
    respondWith { orderSubsystem.orderViews[V](query) }

  def orderTreeComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery) =
    for (snapshot ← ordersComplementedBy[V](query))
      yield for (o ← snapshot)
        yield OrderTreeComplemented.fromOrderComplemented(query.jobChainQuery.pathQuery.folderPath, o)

  def ordersComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery) =
    respondWith {
      complementOrders(orderSubsystem.orderViews[V](query))
    }

  private def complementOrders[V <: OrderView: OrderView.Companion](views: immutable.Seq[V]) = {
    val nodeOverviews = {
      val jobChainPathToNodeKeys = (views map { _.nodeKey }).distinct groupBy { _.jobChainPath }
      for ((jobChainPath, nodeKeys) ← jobChainPathToNodeKeys.toVector.sortBy { _._1 };
           jobChain ← orderSubsystem.jobChainOption(jobChainPath).iterator;
           nodeKey ← nodeKeys;
           node ← (jobChain.nodeMap.get(nodeKey.nodeId) collect { case n: JobNode ⇒ n.overview }).toArray sortBy { _.nodeId })  // sort just for determinism - not the original node order
        yield node
    }
    val jobChainOverviews = (nodeOverviews map { _.jobChainPath }).distinct.sorted flatMap orderSubsystem.jobChainOption map { _.overview }
    val jobs = {
      val jobPaths = (nodeOverviews map { _.jobPath }).distinct
      jobPaths flatMap jobSubsystem.fileBasedOption
    }
    val tasks = views map { _.orderProcessingState } collect {
      case inTask: OrderProcessingState.InTask ⇒ taskSubsystem.task(inTask.taskId)
    }
    val processClasses = {
      val taskProcessClasses = tasks flatMap { _.processClassOption }
      val jobProcessClasses = (jobs flatMap { _.defaultProcessClassPathOption }).distinct flatMap processClassSubsystem.fileBasedOption
      (taskProcessClasses ++ jobProcessClasses).distinct
    }
    OrdersComplemented(
      views,
      jobChainOverviews,
      nodeOverviews,
      (jobs map { _.overview }).sorted,
      (tasks map { _.overview }).sorted,
      (processClasses map { _.overview }).sorted)
  }

  def orderStatistics(query: JobChainNodeQuery): Future[Snapshot[OrderStatistics]] =
    respondWith {
      orderSubsystem.orderStatistics(query)
    }

  def jobChainOverview(jobChainPath: JobChainPath): Future[Snapshot[JobChainOverview]] =
    respondWith {
      orderSubsystem.jobChain(jobChainPath).overview
    }

  def jobChainOverviewsBy(query: JobChainQuery): Future[Snapshot[Seq[JobChainOverview]]] =
    respondWith {
      (orderSubsystem.jobChainsByQuery(query) map { _.overview }).toVector
    }

  def jobChainDetailed(jobChainPath: JobChainPath): Future[Snapshot[JobChainDetailed]] =
    respondWith {
      orderSubsystem.jobChain(jobChainPath).details
    }

  def jobOverviews: Future[Snapshot[Vector[JobOverview]]] =
    respondWith {
      jobSubsystem.fileBaseds map { _.overview }
    }

  def jobOverview(jobPath: JobPath): Future[Snapshot[JobOverview]] =
    respondWith {
      jobSubsystem.job(jobPath).overview
    }

//  def processClassDetaileds: Future[Snapshot[Vector[ProcessClassDetailed]]] =
//    respondWith {
//      processClassSubsystem.fileBaseds map { _.detailed }
//    }

//  def processClassDetailed(processClassPath: ProcessClassPath): Future[Snapshot[ProcessClassDetailed]] =
//    respondWith {
//      processClassSubsystem.processClass(processClassPath).detailed
//    }

  def processClassOverviews: Future[Snapshot[Vector[ProcessClassOverview]]] =
    respondWith {
      processClassSubsystem.fileBaseds map { _.overview }
    }

  def processClassOverview(processClassPath: ProcessClassPath): Future[Snapshot[ProcessClassOverview]] =
    respondWith {
      processClassSubsystem.processClass(processClassPath).overview
    }

  def taskOverview(taskId: TaskId): Future[Snapshot[TaskOverview]] =
    respondWith {
      taskSubsystem.task(taskId).overview
    }

  def agentUris: Future[Snapshot[Set[AgentAddress]]] =
    respondWith {
      (for (processClass ← processClassSubsystem.fileBaseds;
           agentUri ← processClass.agentUris)
        yield agentUri
      ).toSet
    }

  def isKnownAgentUri(uri: AgentAddress): Future[Boolean] =
    directOrSchedulerThreadFuture {
      processClassSubsystem.fileBaseds exists { _ containsAgentUri uri }
    }

  private def respondWith[A](content: ⇒ A): Future[Snapshot[A]] =
    directOrSchedulerThreadFuture {
      // We are in control of the scheduler thread. No hot scheduler events may occur now.
      // eventCollector.newEventId returns a good EventId usable for the event web service.
      Snapshot(eventCollector.newEventId(), content)
    }
}
