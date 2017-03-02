package com.sos.scheduler.engine.client.api

import com.sos.jobscheduler.data.event.Stamped
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import scala.concurrent.Future

/**
  * @author Joacim Zschimmer
  */
trait SchedulerOverviewClient {
  def overview: Future[Stamped[SchedulerOverview]]
}
