package com.sos.scheduler.engine.kernel

import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.common.event.EventIdGenerator
import com.sos.scheduler.engine.common.event.collector.EventCollector
import com.sos.scheduler.engine.data.agent.AgentAddress
import com.sos.scheduler.engine.data.compounds.{OrderTreeComplemented, OrdersComplemented}
import com.sos.scheduler.engine.data.event.Snapshot
import com.sos.scheduler.engine.data.filebased.{FileBasedView, TypedPath}
import com.sos.scheduler.engine.data.job.{JobHistoryEntry, JobOverview, JobPath, JobView, TaskId, TaskOverview}
import com.sos.scheduler.engine.data.jobchain.{JobChainDetailed, JobChainOverview, JobChainPath}
import com.sos.scheduler.engine.data.order.{JocOrderStatistics, OrderKey, OrderProcessingState, OrderView}
import com.sos.scheduler.engine.data.processclass.{ProcessClassOverview, ProcessClassPath, ProcessClassView}
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, JobChainQuery, JobQuery, OrderQuery, PathQuery}
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

  def overview: Future[Snapshot[SchedulerOverview]] =
    respondWithSnapshotFuture { scheduler.overview }

  def fileBased[P <: TypedPath, V <: FileBasedView: FileBasedView.Companion](path: P): Future[Snapshot[V]] =
    respondWithSnapshotFuture {
      fileBasedSubsystemRegister.fileBased(path).fileBasedView[V]
    }

  def fileBaseds[P <: TypedPath: TypedPath.Companion, V <: FileBasedView: FileBasedView.Companion](query: PathQuery): Future[Snapshot[immutable.Seq[V]]] =
    respondWithSnapshotFuture {
      fileBasedSubsystemRegister.fileBaseds[P](query) map { _.fileBasedView[V] }
    }

  def anyTypeFileBaseds[V <: FileBasedView: FileBasedView.Companion](query: PathQuery): Future[Snapshot[immutable.Seq[V]]] =
    respondWithSnapshotFuture {
      for (companion ← fileBasedSubsystemRegister.companions;
           seq ← fileBasedSubsystemRegister.fileBaseds[companion.Path](query)(companion.typedPathCompanion) map { _.fileBasedView[V] })
        yield seq
    }

  def order[V <: OrderView: OrderView.Companion](orderKey: OrderKey) =
    respondWithSnapshotFuture { orderSubsystem.order(orderKey).view[V] }

  def ordersBy[V <: OrderView: OrderView.Companion](query: OrderQuery): Future[Snapshot[immutable.Seq[V]]] =
    respondWithSnapshotFuture { orderSubsystem.orderViews[V](query) }

  def orderTreeComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery) =
    for (snapshot ← ordersComplementedBy[V](query))
      yield for (o ← snapshot)
        yield OrderTreeComplemented.fromOrderComplemented(query.jobChainQuery.pathQuery.folderPath, o)

  def ordersComplementedBy[V <: OrderView: OrderView.Companion](query: OrderQuery) =
    respondWithSnapshotFuture {
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

  def jocOrderStatistics(query: JobChainNodeQuery): Future[Snapshot[JocOrderStatistics]] =
    respondWithSnapshotFuture {
      val (distriChains, localChains) = orderSubsystem.jobChainsByQuery(query.jobChainQuery) partition { _.isDistributed }
      val distributedStatisticsFuture = orderSubsystem.distributedOrderStatistics(query, distriChains)
      val localStatistics = orderSubsystem.nonDistributedOrderStatistics(query, localChains)
      (localStatistics, distributedStatisticsFuture)
    } flatMap { snapshot ⇒
      val Snapshot(eventId, (localStatistics, distributedStatisticsFuture)) = snapshot
      for (distributedStatistics ← distributedStatisticsFuture) yield
        Snapshot(eventId, localStatistics + distributedStatistics)
    }

  def jobChainOverview(jobChainPath: JobChainPath): Future[Snapshot[JobChainOverview]] =
    respondWithSnapshotFuture {
      orderSubsystem.jobChain(jobChainPath).overview
    }

  def jobChainOverviewsBy(query: JobChainQuery): Future[Snapshot[Seq[JobChainOverview]]] =
    respondWithSnapshotFuture {
      (orderSubsystem.jobChainsByQuery(query) map { _.overview }).toVector
    }

  def jobChainDetailed(jobChainPath: JobChainPath): Future[Snapshot[JobChainDetailed]] =
    respondWithSnapshotFuture {
      orderSubsystem.jobChain(jobChainPath).detailed
    }

  def jobChainDetailedBy(query: JobChainQuery): Future[Snapshot[Seq[JobChainDetailed]]] =
    respondWithSnapshotFuture {
      (orderSubsystem.jobChainsByQuery(query) map { _.detailed }).toVector
    }

  def jobs[V <: JobView: JobView.Companion](query: JobQuery = JobQuery.All): Future[Snapshot[Vector[V]]] =
    respondWithSnapshotFuture {
      jobSubsystem.jobsBy(query) map { _.view[V] }
    }

  def job[V <: JobView: JobView.Companion](jobPath: JobPath): Future[Snapshot[V]] =
    respondWithSnapshotFuture {
      jobSubsystem.job(jobPath).view[V]
    }

  def jobsJocOrderStatistics(query: JobQuery, isDistributed: Option[Boolean]): Future[Map[JobPath, JocOrderStatistics]] =
    directOrSchedulerThreadFuture {
      jobSubsystem.jobsBy(query).map(job ⇒ job.path → directJobJocOrderStatistics(job.path, isDistributed)).toMap
    }

  def jobJocOrderStatistics(jobPath: JobPath, isDistributed: Option[Boolean]): Future[JocOrderStatistics] =
    directOrSchedulerThreadFuture {
      directJobJocOrderStatistics(jobPath, isDistributed = isDistributed)
    }

  private def directJobJocOrderStatistics(jobPath: JobPath, isDistributed: Option[Boolean]): JocOrderStatistics = {
    if (!isDistributed.contains(false)) throw new IllegalArgumentException("return=JocOrderStatistics requires isDistributed=false")
    val nodeKeys = jobSubsystem.job(jobPath).nodeKeys
    val (distributedNodeKeys, localNodeKeys) = nodeKeys partition (o ⇒ orderSubsystem.jobChain(o.jobChainPath).isDistributed)
    //val distributedStatisticsFuture = orderSubsystem.distributedOrderStatistics(distributedNodeKeys)
    orderSubsystem.nonDistributedOrderStatistics(localNodeKeys)
  }

  def jobsHistory(jobPath: JobPath, limit: Int): Future[Seq[JobHistoryEntry]] =
    jobSubsystem.jobsHistory(jobPath, limit)

  def jobMatches(jobPath: JobPath, query: JobQuery): Boolean =
    query.pathQuery.matches(jobPath) &&
      jobSubsystem.fileBasedOption(jobPath).exists(o ⇒ query.isInState(o.state))

  def processClass[V <: ProcessClassView: ProcessClassView.Companion](processClassPath: ProcessClassPath): Future[Snapshot[V]] =
    respondWithSnapshotFuture {
      processClassSubsystem.fileBased(processClassPath).view[V]
    }

  def processClasses[V <: ProcessClassView: ProcessClassView.Companion](query: PathQuery): Future[Snapshot[immutable.Seq[V]]] =
    respondWithSnapshotFuture {
      processClassSubsystem.fileBasedsBy(query) map { _.view[V] }
    }

  def taskOverview(taskId: TaskId): Future[Snapshot[TaskOverview]] =
    respondWithSnapshotFuture {
      taskSubsystem.task(taskId).overview
    }

  def taskOverviews(query: PathQuery): Future[Snapshot[immutable.Seq[TaskOverview]]] =
    respondWithSnapshotFuture {
      for (job ← jobSubsystem.fileBasedsBy(query);
           task ← job.tasks) yield
        task.overview
    }

  def agentUris: Future[Snapshot[Set[AgentAddress]]] =
    respondWithSnapshotFuture {
      (for (processClass ← processClassSubsystem.fileBaseds;
           agentUri ← processClass.agentUris)
        yield agentUri
      ).toSet
    }

  def isKnownAgentUri(uri: AgentAddress): Future[Boolean] =
    directOrSchedulerThreadFuture {
      processClassSubsystem.fileBaseds exists { _ containsAgentUri uri }
    }

  private def respondWithSnapshotFuture[A](content: ⇒ A): Future[Snapshot[A]] =
    directOrSchedulerThreadFuture {
      // We are in control of the scheduler thread. No hot scheduler events may occur now.
      // eventCollector.newEventId returns a good EventId usable for the event web service.
      eventIdGenerator.newSnapshot(content)
    }
}
