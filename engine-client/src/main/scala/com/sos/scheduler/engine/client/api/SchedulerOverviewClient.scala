package com.sos.scheduler.engine.client.api

import com.sos.scheduler.engine.data.event.Snapshot
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import scala.concurrent.Future

/**
  * @author Joacim Zschimmer
  */
trait SchedulerOverviewClient {
  def overview: Future[Snapshot[SchedulerOverview]]
}
