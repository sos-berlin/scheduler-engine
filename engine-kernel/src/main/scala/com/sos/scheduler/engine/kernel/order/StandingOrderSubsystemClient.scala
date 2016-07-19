package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystemClient
import javax.inject.{Inject, Singleton}

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class StandingOrderSubsystemClient @Inject private(
  protected val subsystem: StandingOrderSubsystem)
  (implicit protected val schedulerThreadCallQueue: SchedulerThreadCallQueue)
extends FileBasedSubsystemClient {

  def order(orderKey: OrderKey) = fileBased(orderKey)

  def orderOption(orderKey: OrderKey) = fileBasedOption(orderKey)
}
