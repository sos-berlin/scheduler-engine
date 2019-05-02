package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.data.jobchain.{JobChainDetailed, JobChainOverview, JobChainPath}
import com.sos.scheduler.engine.data.order.{OrderDetailed, OrderKey, OrderOverview}
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

  override type ThisFileBased = JobChain

  def tryRemoveOrder(k: OrderKey): Unit = inSchedulerThread { for (o ← subsystem.orderOption(k)) o.remove() }

  def remove(path: JobChainPath): Unit = inSchedulerThread { subsystem.removeJobChain(path) }

  def overview(path: companion.Path): JobChainOverview = inSchedulerThread { subsystem.fileBased(path).overview }

  def detailed(path: companion.Path): JobChainDetailed = inSchedulerThread { subsystem.fileBased(path).detailed }

  @deprecated("Avoid direct access to C++ near objects")
  def jobChain(path: JobChainPath): JobChain = fileBased(path)

  @deprecated("Avoid direct access to C++ near objects")
  def jobChainOption(path: JobChainPath): Option[ThisFileBased] = fileBasedOption(path)

  def orderOverview(orderKey: OrderKey): OrderOverview = inSchedulerThread { subsystem.order(orderKey).overview }

  def orderDetailed(orderKey: OrderKey): OrderDetailed = inSchedulerThread { subsystem.order(orderKey).details }

  @deprecated("Avoid direct access to C++ near objects")
  def order(orderKey: OrderKey): Order = inSchedulerThread { subsystem.order(orderKey) }

  @deprecated("Avoid direct access to C++ near objects")
  def orderOption(orderKey: OrderKey): Option[Order] = inSchedulerThread { subsystem.orderOption(orderKey) }
}
