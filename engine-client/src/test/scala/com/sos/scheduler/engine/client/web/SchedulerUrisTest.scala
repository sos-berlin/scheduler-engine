package com.sos.scheduler.engine.client.web

import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class SchedulerUrisTest extends FreeSpec {

  private val uris = SchedulerUris("http://0.0.0.0:1111/")

  "overview" in {
    assert(uris.overview == "http://0.0.0.0:1111/jobscheduler/master/api")
  }

  "orderOverview" in {
    assert(uris.order.overviews == "http://0.0.0.0:1111/jobscheduler/master/api/order/OrderOverview/")
  }
}
