package com.sos.scheduler.engine.kernel.order

import com.google.inject.Injector
import com.sos.jobscheduler.base.utils.PerKeyLimiter
import com.sos.jobscheduler.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.jobscheduler.common.guice.GuiceImplicits._
import com.sos.jobscheduler.common.scalautil.Collections.emptyToNone
import com.sos.jobscheduler.common.scalautil.SideEffect.ImplicitSideEffect
import com.sos.jobscheduler.data.scheduler.SchedulerId
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeKey}
import com.sos.scheduler.engine.data.order.{JocOrderStatistics, OrderKey, OrderProcessingState, OrderView}
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, JobChainQuery, OrderQuery, PathQuery}
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.cppproxy.{Job_chainC, OrderC, Order_subsystemC}
import com.sos.scheduler.engine.kernel.database.{DatabaseSubsystem, JdbcConnectionPool}
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.job.Job
import com.sos.scheduler.engine.kernel.order.jobchain.{JobChain, Node}
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate._
import com.sos.scheduler.engine.kernel.persistence.hibernate._
import com.typesafe.config.Config
import javax.inject.{Inject, Provider, Singleton}
import javax.persistence.EntityManagerFactory
import scala.collection.{immutable, mutable}
import scala.concurrent.{ExecutionContext, Future}

@ForCpp
@Singleton
final class OrderSubsystem @Inject private(
  protected[this] val cppProxy: Order_subsystemC,
  implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue,
  folderSubsystem: FolderSubsystem,
  databaseSubsystem: DatabaseSubsystem,
  ownClusterMemberIdProvider: Provider[ClusterMemberId],
  jdbcConnectionPool: JdbcConnectionPool,
  schedulerId: SchedulerId,
  databaseOrders: DatabaseOrders,
  config: Config,
  protected val injector: Injector)
  (implicit executionContext: ExecutionContext)
extends FileBasedSubsystem {

  type ThisSubsystemClient = OrderSubsystemClient
  type ThisSubsystem = OrderSubsystem
  type ThisFileBased = JobChain
  type ThisFile_basedC = Job_chainC
  type Path = JobChainPath

  val companion = OrderSubsystem

  private implicit lazy val entityManagerFactory = injector.instance[EntityManagerFactory]
  private lazy val persistentStateStore = injector.getInstance(classOf[HibernateJobChainNodeStore])
  private lazy val ownClusterMemberId = ownClusterMemberIdProvider.get()

  @ForCpp
  private def persistNodeState(node: Node): Unit = {
    transaction { implicit entityManager ⇒
      persistentStateStore.store(node.persistentState)
    }
  }

  private[kernel] def nonDistributedOrderStatistics(query: JobChainNodeQuery, nonDistributedJobChains: TraversableOnce[JobChain]): JocOrderStatistics = {
    var result = new JocOrderStatistics.Mutable
    for (order ← nonDistributedOrderIteratorBy(query, nonDistributedJobChains)) {
      result.count(order.queryable)
    }
    result.toImmutable
  }

  private def nonDistributedOrderIteratorBy(query: JobChainNodeQuery, jobChains: TraversableOnce[JobChain]): Iterator[Order] =
    if (query.matchesCompleteJobChains)
      for (jobChain ← jobChains.toIterator;
           order ← jobChain.orderIterator)
        yield order
    else
      for (jobChain ← jobChains.toIterator;
           node ← jobChain.jobNodes(query);
           order ← jobChain.orderIterator)
        yield order

  private[kernel] def distributedOrderStatistics(query: JobChainNodeQuery, jobChains: TraversableOnce[JobChain]): Future[JocOrderStatistics] = {
    val conditionSqlOption =
      if (query.matchesCompleteJobChains)
        jobChains.nonEmpty option databaseOrders.jobChainPathsToSql(jobChains map { _.path })
      else
        emptyToNone(databaseOrders.nodeKeysToSql(queryToNodeKeys(query, jobChains map { _.path })))
    conditionSqlOption match {
      case Some(conditionSql) ⇒
        fetchDistributedOrderStatistics(query, conditionSql)
      case None ⇒
        Future.successful(JocOrderStatistics.Zero)
    }
  }

  private def fetchDistributedOrderStatistics(query: JobChainNodeQuery, conditionSql: String): Future[JocOrderStatistics] =
    jdbcConnectionPool.readOnly { connection ⇒
      DatabaseOrders.fetchDistributedOrderStatistics(connection, databaseOrders.queryToSql(query, conditionSql))
    }

  private def queryToNodeKeys(query: JobChainNodeQuery, jobChainPaths: TraversableOnce[JobChainPath]): TraversableOnce[NodeKey] =
    for (jobChainPath ← jobChainPaths;
          node ← jobChain(jobChainPath).jobNodes(query))
      yield node.nodeKey

  private[kernel] def orderViews[V <: OrderView: OrderView.Companion](query: OrderQuery): immutable.Seq[V] = {
    val (distriChains, localChains) = jobChainsByQuery(query.nodeQuery.jobChainQuery) partition { _.isDistributed }
    val local = localOrderViews[V](localChains, query)
    val distributed = distributedOrderViews[V](distriChains, query)
    var all: Iterator[V] = local ++ distributed
    for (limit ← query.notInTaskLimitPerNode) {
      val perKeyLimiter = new PerKeyLimiter(limit, (o: OrderView) ⇒ o.nodeKey)
      all = all filter { o ⇒
        classOf[OrderProcessingState.InTask].isAssignableFrom(o.orderProcessingStateClass) || perKeyLimiter(o)
      }
    }
    all.toVector
  }

  private def localOrderViews[V <: OrderView: OrderView.Companion](jobChains: Iterator[JobChain], query: OrderQuery): Iterator[V] = {
    val orders = query.orderKeyOption match {
      case Some(orderKey) ⇒
        // If a single non-distributed Order is specified, we want get an exception (with error message) if it does not exists.
        Iterator(jobChain(orderKey.jobChainPath).order(orderKey.id))
      case _ ⇒
        val jobChainToOrders = query.orderIds match {
          case Some(orderIds) ⇒ (o: JobChain) ⇒ o.orderIterator(orderIds)
          case None           ⇒ (o: JobChain) ⇒ o.orderIterator
        }
        jobChains flatMap jobChainToOrders
      }
    orders filter { o ⇒ query.matchesOrder(o.queryable, o.jobPathOption) } map { _.view[V] }
  }

  private object distributedOrderViews {
    def apply[V <: OrderView: OrderView.Companion](jobChains: Iterator[JobChain], query: OrderQuery): Seq[V] =
      if (jobChains.isEmpty || (query.orderIds/*Option*/ exists { _.isEmpty }))
        Nil
      else {
        val result = mutable.Buffer[V]()
        cppProxy.java_for_each_distributed_order(
          jobChainPaths = toJavaArrayList((jobChains map { _.path.withoutStartingSlash }).toSeq),
          orderIds = (query.orderIds map { o ⇒ toJavaArrayList(o map { _.string }) }).orNull,
          perNodeLimit = query.notInTaskLimitPerNode getOrElse Int.MaxValue,
          new OrderCallback {
            def apply(orderC: OrderC) = {
              val order = orderC.getSister
              if (query.matchesOrder(order.queryable, order.jobPathOption)) {
                result += order.view[V]
              }
            }
          })
        result map replaceWithOwnOrderView[V]
      }

    private def toJavaArrayList[A <: AnyRef](elements: Iterable[A]): java.util.ArrayList[AnyRef] =
      new java.util.ArrayList[AnyRef](elements.size) sideEffect { a ⇒
        for (e ← elements) a.add(e)
      }

    private def replaceWithOwnOrderView[V <: OrderView: OrderView.Companion](o: V): V =
      if (o.occupyingClusterMemberId contains ownClusterMemberId)
        orderOption(o.orderKey) map { _.view[V] } getOrElse o
      else
        o
  }

  private[order] def localOrderIterator: Iterator[Order] =
    jobChainsByQuery(JobChainQuery.All) flatMap { _.orderIterator }

  def order(orderKey: OrderKey): Order =
    inSchedulerThread {
      jobChain(orderKey.jobChainPath).order(orderKey.id)
    }

  def orderOption(orderKey: OrderKey): Option[Order] =
    inSchedulerThread {
      jobChain(orderKey.jobChainPath).orderOption(orderKey.id)
    }

  def removeJobChain(o: JobChainPath): Unit =
    inSchedulerThread {
      jobChain(o).remove()
    }

  def jobChainsOfJob(job: Job): Vector[JobChain] =
    inSchedulerThread {
      (fileBasedIterator filter { _ refersToJob job }).toVector
    }

  private[kernel] def jobChainsByQuery(query: JobChainQuery): Iterator[JobChain] =
    query.pathQuery match {
      case PathQuery.All ⇒
        val reducedQuery = query.copy(pathQuery = PathQuery.All)
        orderedVisibleFileBasedIterator filter { o ⇒ reducedQuery matches o.queryable }
      case PathQuery.Folder(folderPath, ignoringIsRecursive) ⇒
        folderSubsystem.requireExistence(folderPath)
        orderedVisibleFileBasedIterator filter { o ⇒ query matches o.queryable }
      case single: PathQuery.SinglePath ⇒
        Iterator(jobChain(single.as[JobChainPath])) filter { o ⇒ query matches o.queryable }
    }

  def jobChain(o: JobChainPath): JobChain =
    inSchedulerThread {
      fileBased(o)
    }

  def jobChainOption(o: JobChainPath): Option[JobChain] =
    inSchedulerThread {
      fileBasedOption(o)
    }
}

object OrderSubsystem extends
FileBasedSubsystem.AbstractCompanion[OrderSubsystemClient, OrderSubsystem, JobChainPath, JobChain] {

  val fileBasedType = FileBasedType.JobChain
  val stringToPath = JobChainPath.apply _
}
