package com.sos.scheduler.engine.kernel.order

import com.google.inject.Injector
import com.sos.scheduler.engine.common.inject.GuiceImplicits._
import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.data.scheduler.ClusterMemberId
import com.sos.scheduler.engine.kernel.cppproxy.Order_subsystemC
import com.sos.scheduler.engine.kernel.job.Job
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain
import com.sos.scheduler.engine.kernel.persistence.hibernate._
import com.sos.scheduler.engine.kernel.scheduler.Subsystem
import javax.inject.{Singleton, Inject}
import javax.persistence.EntityManagerFactory
import scala.collection.JavaConversions._

@Singleton
final class OrderSubsystem @Inject private(cppProxy: Order_subsystemC) extends Subsystem {

  private[order] lazy val clusterMemberId = injector.apply[ClusterMemberId]
  private[order] lazy val entityManagerFactory = injector.apply[EntityManagerFactory]
  private[order] lazy val orderStore = injector.apply[HibernateOrderStore]
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
    for (o <- orderOption(k))
      o.remove()
  }

  def order(orderKey: OrderKey): Order =
    jobChain(orderKey.jobChainPath).order(orderKey.id)

  def orderOption(orderKey: OrderKey): Option[Order] =
    jobChain(orderKey.jobChainPath).orderOption(orderKey.id)

  def removeJobChain(o: JobChainPath) {
    jobChain(o).remove()
  }

  def jobChainsOfJob(job: Job): Iterable[JobChain] =
    jobChains filter { _ refersToJob job }

  def jobChains: Seq[JobChain] =
    cppProxy.java_file_baseds

  def jobChain(o: JobChainPath): JobChain =
    jobChainOption(o) getOrElse sys.error(s"Unknown $o")

  def jobChainOption(o: JobChainPath): Option[JobChain] =
    Option(cppProxy.java_file_based_or_null(o.string))
}
