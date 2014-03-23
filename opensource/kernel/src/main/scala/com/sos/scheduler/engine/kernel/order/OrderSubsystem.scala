package com.sos.scheduler.engine.kernel.order

import com.google.inject.Injector
import com.sos.scheduler.engine.common.inject.GuiceImplicits._
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.cppproxy.{Job_chainC, Order_subsystemC}
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import com.sos.scheduler.engine.kernel.job.Job
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain
import com.sos.scheduler.engine.kernel.persistence.hibernate._
import javax.inject.{Singleton, Inject}
import javax.persistence.EntityManagerFactory

@Singleton
final class OrderSubsystem @Inject private(
  protected[this] val cppProxy: Order_subsystemC, injector: Injector,
  implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue)
extends FileBasedSubsystem {

  type MySubsystem = OrderSubsystem
  type MyFileBased = JobChain
  type MyFile_basedC = Job_chainC

  val companion = OrderSubsystem

  private[order] lazy val clusterMemberId = injector.apply[ClusterMemberId]
  private[order] lazy val entityManagerFactory = injector.apply[EntityManagerFactory]
  private[order] lazy val jobChainStore = injector.apply[HibernateJobChainStore]
  private[order] lazy val jobChainNodeStore = injector.apply[HibernateJobChainNodeStore]

//  def jobChainMap = new Map[JobChainPath, JobChain] {
//    def get(key: JobChainPath) =
//      jobChainOption(key)
//
//    def iterator =
//      jobChains.iterator map { o => o.path -> o }
//
//    def -(key: JobChainPath) =
//      throw new NotImplementedError
//
//    def +[B1 >: JobChain](kv: (JobChainPath, B1)) =
//      throw new NotImplementedError
//  }

  def tryRemoveOrder(k: OrderKey) {
    inSchedulerThread {
      for (o <- orderOption(k))
        o.remove()
    }
  }

  def order(orderKey: OrderKey): Order =
    jobChain(orderKey.jobChainPath).order(orderKey.id)

  def orderOption(orderKey: OrderKey): Option[Order] =
    jobChain(orderKey.jobChainPath).orderOption(orderKey.id)

  def removeJobChain(o: JobChainPath) {
    jobChain(o).remove()
  }

  def jobChainsOfJob(job: Job): Iterable[JobChain] =
    inSchedulerThread {
      jobChains filter { _ refersToJob job }
    }

  def jobChains: Seq[JobChain] =
    fileBaseds

  def jobChain(o: JobChainPath): JobChain =
    fileBased(o)

  def jobChainOption(o: JobChainPath): Option[JobChain] =
    fileBasedOption(o)
}


object OrderSubsystem extends FileBasedSubsystem.Companion[OrderSubsystem, JobChainPath, JobChain](FileBasedType.jobChain, JobChainPath.apply)
