package com.sos.scheduler.engine.kernel.order

import com.google.inject.Injector
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.folder.FolderPath
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, JobChainQuery}
import com.sos.scheduler.engine.data.order.{OrderKey, OrderOverview, OrderQuery}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.cppproxy.{Job_chainC, Order_subsystemC}
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.kernel.folder.FolderSubsystem
import com.sos.scheduler.engine.kernel.job.Job
import com.sos.scheduler.engine.kernel.order.jobchain.{JobChain, Node}
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate._
import com.sos.scheduler.engine.kernel.persistence.hibernate._
import javax.inject.{Inject, Singleton}
import javax.persistence.EntityManagerFactory
import scala.collection.immutable

@ForCpp
@Singleton
private[kernel] final class OrderSubsystem @Inject private(
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

  private[kernel] def orderOverviews(query: OrderQuery): immutable.Seq[OrderOverview] =
    ordersByQuery(query).toVector map { _.overview }

  private def ordersByQuery(query: OrderQuery) = {
    val q = query.copy(jobChainQuery = JobChainQuery.All)
    jobChainsByQuery(query.jobChainQuery) flatMap { _.orders } filter q.matches
  }

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

  def jobChainsOfJob(job: Job): Iterable[JobChain] =
    inSchedulerThread {
      jobChains filter { _ refersToJob job }
    }

  private[kernel] def jobChainsByQuery(query: JobChainQuery): Seq[JobChain] =
    query.reduce match {
      case JobChainQuery.All ⇒ visibleFileBaseds
      case jobChainPath: JobChainPath ⇒ List(jobChain(jobChainPath))
      case folderPath: FolderPath ⇒
        folderSubsystem.fileBased(folderPath)  // Fails if folders does not exists
        visibleFileBaseds filter { o ⇒ query.matches(o.path) }
    }

  private def jobChains: Vector[JobChain] = fileBaseds

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
