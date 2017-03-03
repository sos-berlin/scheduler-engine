package com.sos.scheduler.engine.kernel

import com.sos.jobscheduler.common.event.EventIdGenerator
import com.sos.jobscheduler.common.event.collector.EventCollector
import com.sos.jobscheduler.data.agent.AgentAddress
import com.sos.jobscheduler.data.event.Stamped
import com.sos.jobscheduler.data.filebased.TypedPath
import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.filebased.FileBasedView
import com.sos.scheduler.engine.data.job.{JobOverview, JobPath, JobView, TaskOverview}
import com.sos.scheduler.engine.data.jobchain.{JobChainDetailed, JobChainOverview, JobChainPath}
import com.sos.scheduler.engine.data.order.{JocOrderStatistics, OrderKey, OrderProcessingState, OrderView}
import com.sos.scheduler.engine.data.processclass.{ProcessClassOverview, ProcessClassPath, ProcessClassView}
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, JobChainQuery, OrderQuery, PathQuery}
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.event.DirectEventClient
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
  protected val eventIdGenerator: EventIdGenerator,
  protected val eventCollector: EventCollector)(
  implicit schedulerThreadCallQueue: SchedulerThreadCallQueue,
  protected val executionContext: ExecutionContext)
extends SchedulerClient with DirectCommandClient with DirectEventClient with DirectOrderClient {

  def overview: Future[Stamped[SchedulerOverview]] =
    respondWithStampedFuture { scheduler.overview }

  def fileBased[P <: TypedPath, V <: FileBasedView: FileBasedView.Companion](path: P): Future[Stamped[V]] =
    respondWithStampedFuture {
      fileBasedSubsystemRegister.fileBased(path).fileBasedView[V]
    }

  def fileBaseds[P <: TypedPath: TypedPath.Companion, V <: FileBasedView: FileBasedView.Companion](query: PathQuery): Future[Stamped[immutable.Seq[V]]] =
    respondWithStampedFuture {
      fileBasedSubsystemRegister.fileBaseds[P](query) map { _.fileBasedView[V] }
    }

  def anyTypeFileBaseds[V <: FileBasedView: FileBasedView.Companion](query: PathQuery): Future[Stamped[immutable.Seq[V]]] =
    respondWithStampedFuture {
      for (companion ← fileBasedSubsystemRegister.companions;
           seq ← fileBasedSubsystemRegister.fileBaseds[companion.Path](query)(companion.typedPathCompanion) map { _.fileBasedView[V] })
        yield seq
    }

  def order[V <: OrderView: OrderView.Companion](orderKey: OrderKey) =
    respondWithStampedFuture { orderSubsystem.order(orderKey).view[V] }

  def ordersBy[V <: OrderView: OrderView.Companion](query: OrderQuery): Future[Stamped[immutable.Seq[V]]] =
    respondWithStampedFuture { orderSubsystem.orderViews[V](query) }

  def orderTreeComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery) =
    for (stamped ← ordersComplementedBy[V](query))
      yield for (o ← stamped)
        yield OrderTreeComplemented.fromOrderComplemented(query.jobChainQuery.pathQuery.folderPath, o)

  def ordersComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery) =
    respondWithStampedFuture {
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
    val jobChainOverviews = {
      val jobChainPaths = (nodeOverviews map { _.jobChainPath }) ++ (views flatMap { _.outerJobChainPath })
      jobChainPaths.distinct.sorted flatMap orderSubsystem.jobChainOption map { _.overview }
    }
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
      (jobs map { _.view[JobOverview] }).sorted,
      (tasks map { _.overview }).sorted,
      (processClasses map { _.view[ProcessClassOverview] }).sorted)
  }

  def jocOrderStatistics(query: JobChainNodeQuery): Future[Stamped[JocOrderStatistics]] =
    respondWithStampedFuture {
      val (distriChains, localChains) = orderSubsystem.jobChainsByQuery(query.jobChainQuery) partition { _.isDistributed }
      val distributedStatisticsFuture = orderSubsystem.distributedOrderStatistics(query, distriChains)
      val localStatistics = orderSubsystem.nonDistributedOrderStatistics(query, localChains)
      (localStatistics, distributedStatisticsFuture)
    } flatMap { stamped ⇒
      val Stamped(eventId, (localStatistics, distributedStatisticsFuture)) = stamped
      for (distributedStatistics ← distributedStatisticsFuture) yield
        Stamped(eventId, localStatistics + distributedStatistics)
    }

  def jobChainOverview(jobChainPath: JobChainPath): Future[Stamped[JobChainOverview]] =
    respondWithStampedFuture {
      orderSubsystem.jobChain(jobChainPath).overview
    }

  def jobChainOverviewsBy(query: JobChainQuery): Future[Stamped[Seq[JobChainOverview]]] =
    respondWithStampedFuture {
      (orderSubsystem.jobChainsByQuery(query) map { _.overview }).toVector
    }

  def jobChainDetailed(jobChainPath: JobChainPath): Future[Stamped[JobChainDetailed]] =
    respondWithStampedFuture {
      orderSubsystem.jobChain(jobChainPath).details
    }

  def jobs[V <: JobView: JobView.Companion](query: PathQuery = PathQuery.All): Future[Stamped[Vector[V]]] =
    respondWithStampedFuture {
      jobSubsystem.fileBasedsBy(query) map { _.view[V] }
    }

  def job[V <: JobView: JobView.Companion](jobPath: JobPath): Future[Stamped[V]] =
    respondWithStampedFuture {
      jobSubsystem.job(jobPath).view[V]
    }

  def processClass[V <: ProcessClassView: ProcessClassView.Companion](processClassPath: ProcessClassPath): Future[Stamped[V]] =
    respondWithStampedFuture {
      processClassSubsystem.fileBased(processClassPath).view[V]
    }

  def processClasses[V <: ProcessClassView: ProcessClassView.Companion](query: PathQuery): Future[Stamped[immutable.Seq[V]]] =
    respondWithStampedFuture {
      processClassSubsystem.fileBasedsBy(query) map { _.view[V] }
    }

  def taskOverview(taskId: TaskId): Future[Stamped[TaskOverview]] =
    respondWithStampedFuture {
      taskSubsystem.task(taskId).overview
    }

  def taskOverviews(query: PathQuery): Future[Stamped[immutable.Seq[TaskOverview]]] =
    respondWithStampedFuture {
      for (job ← jobSubsystem.fileBasedsBy(query);
           task ← job.tasks) yield
        task.overview
    }

  def agentUris: Future[Stamped[Set[AgentAddress]]] =
    respondWithStampedFuture {
      (for (processClass ← processClassSubsystem.fileBaseds;
           agentUri ← processClass.agentUris)
        yield agentUri
      ).toSet
    }

  def isKnownAgentUri(uri: AgentAddress): Future[Boolean] =
    directOrSchedulerThreadFuture {
      processClassSubsystem.fileBaseds exists { _ containsAgentUri uri }
    }

  private def respondWithStampedFuture[A](content: ⇒ A): Future[Stamped[A]] =
    directOrSchedulerThreadFuture {
      // We are in control of the scheduler thread. No hot scheduler events may occur now.
      // eventCollector.newEventId returns a good EventId usable for the event web service.
      eventIdGenerator.stamp(content)
    }
}
