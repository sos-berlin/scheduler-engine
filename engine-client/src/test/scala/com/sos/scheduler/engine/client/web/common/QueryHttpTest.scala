package com.sos.scheduler.engine.client.web.common

import com.sos.jobscheduler.base.serial.PathAndParameterSerializable.{fromPathAndParameters, toPathAndParameters}
import com.sos.jobscheduler.common.sprayutils.SprayUtils.pathSegments
import com.sos.jobscheduler.data.folder.FolderPath
import com.sos.scheduler.engine.client.web.common.QueryHttp.{jobChainQuery, orderQuery}
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderProcessingState, OrderSourceType}
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, JobChainQuery, OrderQuery, PathQuery}
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
final class QueryHttpTest extends FreeSpec with ScalatestRouteTest {

  "JobChainQuery" - {
    def route(expected: ⇒ JobChainQuery) =
      pathSegments("prefix") {
        jobChainQuery { query: JobChainQuery ⇒
          assert(query == expected)
          complete(OK)
        }
      }

    "Missing slash" in {
      Get("/prefix") ~> route(JobChainQuery.All) ~> check {
        assert(!handled)
      }
    }

    "JobChainQuery.All" in {
      Get("/prefix/") ~> route(JobChainQuery.All) ~> check {
        assert(status == OK)
      }
    }

    "JobChainQuery /a/b" in {
      Get("/prefix/a/b") ~> route(JobChainQuery(PathQuery[JobChainPath]("/a/b"))) ~> check {
        assert(status == OK)
      }
    }

    "JobChainQuery /a,1" in {
      Get("/prefix/a,1") ~>
        route(sys.error("SHOULD BE REJECTED")) ~>
        check {
          assert(!handled)
          assert(rejection.toString contains "Comma not allowed")
        }
    }

    "JobChainQuery isDistributed" in {
      Get("/prefix/?isDistributed=true") ~>
        route(JobChainQuery(isDistributed = Some(true))) ~>
        check {
          assert(status == OK)
        }
    }

    "withoutPathToMap, fromUriPathAndParameters" in {
      checkQuery(JobChainQuery(), Uri.Path("/"), Map())
      checkQuery(JobChainQuery(PathQuery(FolderPath("/FOLDER")), isDistributed = Some(true)), Uri.Path("/FOLDER/"), Map("isDistributed" → "true"))
      checkQuery(JobChainQuery(PathQuery(JobChainPath("/JOBCHAIN")), isDistributed = Some(false)), Uri.Path("/JOBCHAIN"), Map("isDistributed" → "false"))
      checkQuery(
       JobChainQuery(
          PathQuery(FolderPath("/FOLDER"), isRecursive = false)
        ),
        Uri.Path("/FOLDER/*"),
        Map())
      checkQuery(JobChainQuery(), Uri.Path("/"), Map())
    }

    def checkQuery(jobChainQuery: JobChainQuery, path: Uri.Path, parameters: Map[String, String]) = {
      assert(toPathAndParameters(jobChainQuery) == ((path.toString, parameters)))
      assert(fromPathAndParameters[JobChainQuery]((path.toString, Map(parameters.toVector: _*))) == jobChainQuery)
    }
  }

  "OrderQuery" - {
    def route(expected: ⇒ OrderQuery) =
      pathSegments("prefix") {
        orderQuery { query ⇒
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
      Get("/prefix/a/b") ~> route(OrderQuery(JobChainNodeQuery(jobChainQuery = JobChainQuery(pathQuery = PathQuery[JobChainPath]("/a/b"))))) ~> check {
        assert(status == OK)
      }
    }

    "OrderQuery /a?orderId=1" in {
      Get("/prefix/a?orderIds=1") ~>
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

    "OrderQuery /?jobPaths=/A,/B" in {
      Get("/prefix/?jobPaths=/A,/B") ~>
        route(OrderQuery(JobChainNodeQuery(jobPaths = Some(Set(JobPath("/A"), JobPath("/B")))))) ~>
        check {
          assert(status == OK)
        }
    }

    "OrderQuery isDistributed" in {
      Get("/prefix/?isDistributed=true") ~>
        route(OrderQuery(JobChainNodeQuery(JobChainQuery(isDistributed = Some(true))))) ~>
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

    "OrderQuery orIsSuspended" in {
      Get("/prefix/?orIsSuspended=true") ~>
        route(OrderQuery(orIsSuspended = true)) ~>
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
          PathQuery[JobChainPath]("/a/"),
          isSuspended = Some(true),
          isBlacklisted = Some(false))) ~>
        check {
          assert(status == OK)
        }
    }

    "withoutPathToMap, fromUriPathAndParameters" in {
      checkQuery(OrderQuery(), Uri.Path("/"), Map())
      checkQuery(OrderQuery(JobChainQuery(PathQuery(FolderPath("/FOLDER"))), isSuspended = Some(true)), Uri.Path("/FOLDER/"), Map("isSuspended" → "true"))
      checkQuery(OrderQuery(JobChainQuery(PathQuery(JobChainPath("/JOBCHAIN"))), isSuspended = Some(false)), Uri.Path("/JOBCHAIN"), Map("isSuspended" → "false"))
      checkQuery(
        OrderQuery(
          JobChainQuery(PathQuery(FolderPath("/FOLDER"), isRecursive = false)),
          isSuspended = Some(false),
          isOrderSourceType = Some(Set(OrderSourceType.AdHoc, OrderSourceType.Permanent)),
          isOrderProcessingState = Some(Set(OrderProcessingState.NotPlanned.getClass, classOf[OrderProcessingState.InTaskProcess]))
        ),
        Uri.Path("/FOLDER/*"),
        Map("isSuspended" → "false", "isOrderSourceType" → "AdHoc,Permanent", "isOrderProcessingState" → "NotPlanned,InTaskProcess"))  // Incidentally, Scala Set with two elements retains orders
      checkQuery(OrderQuery(notInTaskLimitPerNode = Some(123)), Uri.Path("/"), Map("notInTaskLimitPerNode" → "123"))
    }

    def checkQuery(orderQuery: OrderQuery, path: Uri.Path, parameters: Map[String, String]) = {
      assert(toPathAndParameters(orderQuery) == ((path.toString, parameters)))
      assert(fromPathAndParameters[OrderQuery]((path.toString, Map(parameters.toVector: _*))) == orderQuery)
    }
  }
}
