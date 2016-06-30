package com.sos.scheduler.engine.kernel

import com.sos.scheduler.engine.client.api.SchedulerClient
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import javax.inject.{Inject, Singleton}

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class DirectSchedulerClient @Inject private(scheduler: Scheduler, orderSubsystem: OrderSubsystem)
extends SchedulerClient{

  def overview = scheduler.overview

  def orderOverviews = orderSubsystem.orderOverviews
}
