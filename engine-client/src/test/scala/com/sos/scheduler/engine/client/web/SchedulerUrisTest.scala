package com.sos.scheduler.engine.client.web

import com.sos.scheduler.engine.data.order.{OrderQuery, OrderSourceType}
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

  "order.overview" in {
    assert(uris.order.overviews(OrderQuery.All) == "http://0.0.0.0:1111/jobscheduler/master/api/order/OrderOverview/")
    assert(uris.order.overviews(OrderQuery(isSuspended = Some(true))) ==
      "http://0.0.0.0:1111/jobscheduler/master/api/order/OrderOverview/?suspended=true")
    assert(uris.order.overviews(OrderQuery(isSuspended = Some(true), isBlacklisted = Some(false))) ==
      "http://0.0.0.0:1111/jobscheduler/master/api/order/OrderOverview/?suspended=true&blacklisted=false")
  }

  "order.fullOverview" in {
    assert(uris.order.fullOverview(OrderQuery.All) == "http://0.0.0.0:1111/jobscheduler/master/api/order/OrdersFullOverview")
    assert(uris.order.fullOverview(OrderQuery(isSuspended = Some(true))) ==
      "http://0.0.0.0:1111/jobscheduler/master/api/order/OrdersFullOverview?suspended=true")
    assert(uris.order.fullOverview(OrderQuery(
      isSuspended = Some(true),
      isBlacklisted = Some(false),
      isSourceType = Some(Set(OrderSourceType.fileOrderSource, OrderSourceType.adHoc)))) ==
      "http://0.0.0.0:1111/jobscheduler/master/api/order/OrdersFullOverview?suspended=true&blacklisted=false&sourceType=fileOrderSource,adHoc")
  }
}
