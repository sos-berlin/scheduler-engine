package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.data.jobchain.{JobChainDetails, JobChainPath}
import com.sos.scheduler.engine.data.order.{OrderKey, OrderOverview}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystemClient
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain
import javax.inject.{Inject, Singleton}

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class OrderSubsystemClient @Inject private(
  protected val subsystem: OrderSubsystem)
  (implicit protected val schedulerThreadCallQueue: SchedulerThreadCallQueue)
extends FileBasedSubsystemClient {

  override def fileBasedDetails(path: JobChainPath): JobChainDetails =
    inSchedulerThread {
      subsystem.jobChain(path).details
    }

  override type ThisFileBased = JobChain

  def tryRemoveOrder(k: OrderKey): Unit = inSchedulerThread { for (o ← subsystem.orderOption(k)) o.remove() }

  def remove(path: JobChainPath): Unit = inSchedulerThread { subsystem.removeJobChain(path) }

  override def details(path: companion.Path): JobChainDetails = inSchedulerThread { subsystem.fileBased(path).details }

  def jobChain(path: JobChainPath): JobChain = fileBased(path)

  def jobChainOption(path: JobChainPath): Option[ThisFileBased] = fileBasedOption(path)

  def orderOverview(orderKey: OrderKey): OrderOverview = inSchedulerThread { subsystem.order(orderKey).overview }

  def order(orderKey: OrderKey): Order = inSchedulerThread { subsystem.order(orderKey) }

  def orderOption(orderKey: OrderKey): Option[Order] = inSchedulerThread { subsystem.orderOption(orderKey) }
}
