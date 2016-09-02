package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.client.api.OrderClient
import com.sos.scheduler.engine.data.event.Snapshot
import com.sos.scheduler.engine.data.order.OrderStatistics
import scala.concurrent.Future

/**
  * @author Joacim Zschimmer
  */
trait DirectOrderClient extends OrderClient {

  def orderStatistics: Future[Snapshot[OrderStatistics]]
}
