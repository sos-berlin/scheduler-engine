package com.sos.scheduler.engine.client.web.order

import com.sos.scheduler.engine.client.web.order.OrderQueryHttp._
import com.sos.scheduler.engine.data.order.{OrderQuery, OrderSourceType}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class OrderQueryHttpTest extends FreeSpec {

  "fromHttpQueryMap" in {
    assert(fromHttpQuery(Map("unknown" → "true")).isLeft)
    assert(fromHttpQuery(Map("suspended" → "true")) == Right(OrderQuery(isSuspended = Some(true))))  // recommended case
    assert(fromHttpQuery(Map("suspended" → "TRUE")) == Right(OrderQuery(isSuspended = Some(true))))  // Due to .toBoolean, not recommended
    assert(fromHttpQuery(Map("suspended" → "T")).isLeft)
    assert(fromHttpQuery(Map("suspended" → "1")).isLeft)
  }

  "toHttpQueryMap, fromHttpQueryMap" in {
    checkQuery(OrderQuery(), Map())
    checkQuery(OrderQuery(isSuspended = Some(true)), Map("suspended" → "true"))
    checkQuery(OrderQuery(isSuspended = Some(false)), Map("suspended" → "false"))
    checkQuery(OrderQuery(isSuspended = Some(false), isSourceType = Some(Set(OrderSourceType.adHoc, OrderSourceType.fileBased))), Map("suspended" → "false", "sourceType" → "adHoc,fileBased"))
  }

  private def checkQuery(orderQuery: OrderQuery, parameters: Map[String, String]) = {
    assert(toHttpQueryMap(orderQuery) == parameters)
    assert(fromHttpQuery(parameters) == Right(orderQuery))
  }
}
