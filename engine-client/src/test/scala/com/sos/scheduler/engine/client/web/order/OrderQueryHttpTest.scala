package com.sos.scheduler.engine.client.web.order

import com.sos.scheduler.engine.client.web.order.OrderQueryHttp._
import com.sos.scheduler.engine.client.web.order.OrderQueryHttp.directives.extendedOrderQuery
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderProcessingState, OrderSourceType}
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
    def route(expected: ⇒ OrderQuery) =
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
      Get("/prefix/a/b") ~> route(OrderQuery(jobChainPathQuery = PathQuery[JobChainPath]("/a/b"))) ~> check {
        assert(status == OK)
      }
    }

    "OrderQuery /a?orderId=1" in {
      Get("/prefix/a?orderId=1") ~>
        route(OrderQuery.All.withOrderKey(JobChainPath("/a") orderKey "1")) ~>
        check {
          assert(status == OK)
        }
    }

    "OrderQuery /a,1" in {
      Get("/prefix/a,1") ~>
        route(sys.error("SHOULD BE REJECTED")) ~>
        check {
          assert(!handled)
          assert(rejection.toString contains "Comma not allowed")
        }
    }

    "OrderQuery isDistributed" in {
      Get("/prefix/?isDistributed=true") ~>
        route(OrderQuery(isDistributed = Some(true))) ~>
        check {
          assert(status == OK)
        }
    }

    "OrderQuery isSuspended" in {
      Get("/prefix/?isSuspended=true") ~>
        route(OrderQuery(isSuspended = Some(true))) ~>
        check {
          assert(status == OK)
        }
    }

    "OrderQuery isSetback" in {
      Get("/prefix/?isSetback=true") ~>
        route(OrderQuery(isSetback = Some(true))) ~>
        check {
          assert(status == OK)
        }
    }

    "OrderQuery isOrderSourceType" in {
      Get("/prefix/?isOrderSourceType=AdHoc,Permanent") ~>
        route(OrderQuery(isOrderSourceType = Some(Set(OrderSourceType.AdHoc, OrderSourceType.Permanent)))) ~>
        check {
          assert(status == OK)
        }
    }

    "OrderQuery isOrderProcessingState" in {
      Get("/prefix/?isOrderProcessingState=NotPlanned,InTaskProcess") ~>
        route(OrderQuery(isOrderProcessingState = Some(Set(OrderProcessingState.NotPlanned.getClass, classOf[OrderProcessingState.InTaskProcess])))) ~>
        check {
          assert(status == OK)
        }
    }

    "OrderQuery isBlacklisted" in {
      Get("/prefix/?isBlacklisted=true") ~>
        route(OrderQuery(isBlacklisted = Some(true))) ~>
        check {
          assert(status == OK)
        }
    }

    "OrderQuery /a/ suspended but not blacklisted" in {
      Get("/prefix/a/?isSuspended=true&isBlacklisted=false") ~>
        route(OrderQuery(
          jobChainPathQuery = PathQuery[JobChainPath]("/a/"),
          isSuspended = Some(true),
          isBlacklisted = Some(false))) ~>
        check {
          assert(status == OK)
        }
    }
  }

  "orderQueryToMap, fromHttpQueryMap" in {
    checkQuery(OrderQuery(), Map())
    checkQuery(OrderQuery(isSuspended = Some(true)), Map("isSuspended" → "true"))
    checkQuery(OrderQuery(isSuspended = Some(false)), Map("isSuspended" → "false"))
    checkQuery(
      OrderQuery(
        isSuspended = Some(false),
        isOrderSourceType = Some(Set(OrderSourceType.AdHoc, OrderSourceType.Permanent)),
        isOrderProcessingState = Some(Set(OrderProcessingState.NotPlanned.getClass, classOf[OrderProcessingState.InTaskProcess]))
      ),
      Map("isSuspended" → "false", "isOrderSourceType" → "AdHoc,Permanent", "isOrderProcessingState" → "NotPlanned,InTaskProcess"))  // Incidentally, Scala Set with two elements retains orders
    checkQuery(OrderQuery(notInTaskLimitPerNode = Some(123)), Map("notInTaskLimitPerNode" → "123"))
  }

  private def checkQuery(orderQuery: OrderQuery, parameters: Map[String, String]) = {
    assert(orderQuery.withoutPathToMap == parameters)
    assert(pathAndParametersToQuery(Uri.Path("/"), Map(parameters.toVector: _*)) == orderQuery)
  }
}
