package com.sos.scheduler.engine.client.web.order

import com.sos.scheduler.engine.client.web.order.OrderQueryHttp._
import com.sos.scheduler.engine.client.web.order.OrderQueryHttp.directives.extendedOrderQuery
import com.sos.scheduler.engine.data.order.OrderSourceType
import com.sos.scheduler.engine.data.queries.{OrderQuery, PathQuery}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.http.StatusCodes.OK
import spray.http.Uri
import spray.routing.Directives._
import spray.testkit.ScalatestRouteTest

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class OrderQueryHttpTest extends FreeSpec with ScalatestRouteTest {

  "OrderQuery" - {
    def route(expected: OrderQuery) =
      pathPrefix("prefix") {
        extendedOrderQuery { query: OrderQuery ⇒
          assert(query == expected)
          complete(OK)
        }
      }

    "Missing slash" in {
      Get("/prefix") ~> route(OrderQuery.All) ~> check {
        assert(!handled)
      }
    }

    "OrderQuery.All" in {
      Get("/prefix/") ~> route(OrderQuery.All) ~> check {
        assert(status == OK)
      }
    }

    "OrderQuery /a/b" in {
      Get("/prefix/a/b") ~> route(OrderQuery(jobChainPathQuery = PathQuery("/a/b"))) ~> check {
        assert(status == OK)
      }
    }

    "OrderQuery /a/ suspended but not blacklisted" in {
      Get("/prefix/a/?suspended=true&blacklisted=false") ~>
        route(OrderQuery(
          jobChainPathQuery = PathQuery("/a/"),
          isSuspended = Some(true),
          isBlacklisted = Some(false))) ~>
        check {
          assert(status == OK)
        }
    }
  }

  "toUriQueryMap, fromHttpQueryMap" in {
    checkQuery(OrderQuery(), Map())
    checkQuery(OrderQuery(isSuspended = Some(true)), Map("suspended" → "true"))
    checkQuery(OrderQuery(isSuspended = Some(false)), Map("suspended" → "false"))
    checkQuery(
      OrderQuery(isSuspended = Some(false), isOrderSourceType = Some(Set(OrderSourceType.adHoc, OrderSourceType.fileBased))),
      Map("suspended" → "false", "sourceType" → "adHoc,fileBased"))  // Incidentally, Scala Set with two elements retains orders
  }

  private def checkQuery(orderQuery: OrderQuery, parameters: Map[String, String]) = {
    assert(toUriQueryMap(orderQuery) == parameters)
    assert(pathAndParametersToQuery(Uri.Path("/"), Map(parameters.toVector: _*)) == orderQuery)
  }
}
