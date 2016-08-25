package com.sos.scheduler.engine.kernel.order

import com.google.inject.Injector
import com.sos.scheduler.engine.base.utils.PerKeyLimiter
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.SideEffect.ImplicitSideEffect
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderDetailed, OrderKey, OrderView}
import com.sos.scheduler.engine.data.queries.{JobChainQuery, OrderQuery, PathQuery}
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.cppproxy.{Job_chainC, OrderC, Order_subsystemC}
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.job.Job
import com.sos.scheduler.engine.kernel.order.jobchain.{JobChain, Node}
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate._
import com.sos.scheduler.engine.kernel.persistence.hibernate._
import javax.inject.{Inject, Provider, Singleton}
import javax.persistence.EntityManagerFactory
import scala.collection.{immutable, mutable}

@ForCpp
@Singleton
final class OrderSubsystem @Inject private(
  protected[this] val cppProxy: Order_subsystemC,
  implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue,
  folderSubsystem: FolderSubsystem,
  ownClusterMemberIdProvider: Provider[ClusterMemberId],
  protected val injector: Injector)
extends FileBasedSubsystem {

  type ThisSubsystemClient = OrderSubsystemClient
  type ThisSubsystem = OrderSubsystem
  type ThisFileBased = JobChain
  type ThisFile_basedC = Job_chainC

  val companion = OrderSubsystem

  private implicit lazy val entityManagerFactory = injector.instance[EntityManagerFactory]
  private lazy val persistentStateStore = injector.getInstance(classOf[HibernateJobChainNodeStore])
  private lazy val ownClusterMemberId = ownClusterMemberIdProvider.get()

  @ForCpp
  private def persistNodeState(node: Node): Unit = {
    transaction { implicit entityManager =>
      persistentStateStore.store(node.persistentState)
    }
  }

  private[kernel] def orderOverview[V <: OrderView: OrderView.Companion](orderKey: OrderKey): V =
   order(orderKey).view[V]

  private[kernel] def orderDetails(orderKey: OrderKey): OrderDetailed =
   order(orderKey).details

  private[kernel] def orderViews[V <: OrderView: OrderView.Companion](query: OrderQuery): immutable.Seq[V] = {
    val (distriChains, localChains) = jobChainsByQuery(query) partition { _.isDistributed }
    val localOrders = localChains flatMap { _.orderIterator } filter { o ⇒ query matchesOrder o.queryable } map { _.view[V] }
    val distriOrders =
      for (o ← distributedOrderViews[V](distriChains, query)) yield
        if (o.occupyingClusterMemberId contains ownClusterMemberId)
          orderOption(o.orderKey) map { _.view[V] } getOrElse o
        else
          o
    var iterator = localOrders ++ distriOrders
    for (limit ← query.notInTaskLimitPerNode) {
      val perKeyLimiter = new PerKeyLimiter(limit, (o: OrderView) ⇒ o.nodeKey)
      iterator = iterator filter { o ⇒ o.processingState.isInTask || perKeyLimiter(o) }
    }
    iterator.toVector
  }

  private def distributedOrderViews[V <: OrderView: OrderView.Companion](jobChains: Iterator[JobChain], query: OrderQuery): Seq[V] = {
    val result = mutable.Buffer[V]()
    val jobChainPathArray = new java.util.ArrayList[AnyRef] sideEffect { a ⇒
      for (jobChain ← jobChains) a.add(jobChain.path.withoutStartingSlash)
    }
    if (!jobChainPathArray.isEmpty) {
      cppProxy.java_for_each_distributed_order(
        jobChainPathArray,
        perNodeLimit = Int.MaxValue,
        callback = new OrderCallback {
          def apply(orderC: OrderC) = {
            val order = orderC.getSister
            if (query matchesOrder order.queryable) {
              result += orderC.getSister.view[V]
            }
          }
        })
    }
    result
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
    query.jobChainPathQuery match {
      case PathQuery.All ⇒
        val reducedQuery = query withJobChainPathQuery PathQuery.All
        orderedVisibleFileBasedIterator filter { o ⇒ reducedQuery matchesJobChain o.queryable }
      case PathQuery.Folder(folderPath) ⇒
        folderSubsystem.requireExistence(folderPath)
        orderedVisibleFileBasedIterator filter { o ⇒ query matchesJobChain o.queryable }
      case single: PathQuery.SinglePath ⇒
        Iterator(jobChain(single.as[JobChainPath])) filter { o ⇒ query matchesJobChain o.queryable }
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
