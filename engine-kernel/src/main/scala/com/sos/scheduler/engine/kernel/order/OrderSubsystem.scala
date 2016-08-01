package com.sos.scheduler.engine.kernel.order

import com.google.inject.Injector
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.folder.FolderPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderKey, OrderOverview}
import com.sos.scheduler.engine.data.queries.{JobChainQuery, OnlyOrderQuery, OrderQuery, PathQuery}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.cppproxy.{Job_chainC, Order_subsystemC}
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.job.Job
import com.sos.scheduler.engine.kernel.order.jobchain.{JobChain, Node, OrderQueueNode}
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate._
import com.sos.scheduler.engine.kernel.persistence.hibernate._
import javax.inject.{Inject, Singleton}
import javax.persistence.EntityManagerFactory
import scala.collection.immutable

@ForCpp
@Singleton
final class OrderSubsystem @Inject private(
  protected[this] val cppProxy: Order_subsystemC,
  implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue,
  folderSubsystem: FolderSubsystem,
  protected val injector: Injector)
extends FileBasedSubsystem {

  type ThisSubsystemClient = OrderSubsystemClient
  type ThisSubsystem = OrderSubsystem
  type ThisFileBased = JobChain
  type ThisFile_basedC = Job_chainC

  val companion = OrderSubsystem

  private implicit lazy val entityManagerFactory = injector.instance[EntityManagerFactory]
  private lazy val persistentStateStore = injector.getInstance(classOf[HibernateJobChainNodeStore])

  @ForCpp
  private def persistNodeState(node: Node): Unit = {
    transaction { implicit entityManager =>
      persistentStateStore.store(node.persistentState)
    }
  }

  private[kernel] def orderOverviews(query: OrderQuery): immutable.Seq[OrderOverview] = {
    val (distriChains, localChains) = jobChainsByQuery(query) partition { _.isDistributed }
    val localOrders = localChains flatMap { _.orderIterator } filter { o ⇒ query matchesOrder o.queryable } map { _.overview }
    val distriOrders = distriChains flatMap distributedOrderOverviews(query)
    (localOrders ++ distriOrders).toVector
  }

  private def distributedOrderOverviews(query: OnlyOrderQuery)(jobChain: JobChain): Iterator[OrderOverview] =
    for (orderQueueNode ← jobChain.nodes.iterator collect { case o: OrderQueueNode ⇒ o };
         orderQueue = orderQueueNode.orderQueue;
         overview ← orderQueue.distributedOrderOverviews(query))
      yield overview

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

  val fileBasedType = FileBasedType.jobChain
  val stringToPath = JobChainPath.apply _
}
