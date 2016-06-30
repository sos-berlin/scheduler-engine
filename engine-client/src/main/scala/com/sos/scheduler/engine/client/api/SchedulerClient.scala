package com.sos.scheduler.engine.client.api

import com.sos.scheduler.engine.data.order.OrderOverview
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import scala.collection.immutable
import scala.concurrent.Future

/**
  * @author Joacim Zschimmer
  */
trait SchedulerClient {

  def overview: Future[SchedulerOverview]

  def orderOverviews: Future[immutable.Seq[OrderOverview]]
}
