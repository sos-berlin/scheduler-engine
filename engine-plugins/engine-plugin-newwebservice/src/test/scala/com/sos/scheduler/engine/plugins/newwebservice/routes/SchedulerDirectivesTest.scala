package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.jobscheduler.common.sprayutils.SprayUtils.pathSegments
import com.sos.jobscheduler.data.folder.FolderPath
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.plugins.newwebservice.routes.SchedulerDirectives._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.routing.Directives._
import spray.testkit.ScalatestRouteTest

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class SchedulerDirectivesTest extends FreeSpec with ScalatestRouteTest {

  "typedPath" - {
    val route =
      pathSegments("job") {
        typedPath(JobPath) { path ⇒
          complete(path.toString)
        }
      } ~
      pathSegments("folder") {
        typedPath(FolderPath) { path ⇒
          complete(path.toString)
        }
      } ~
      pathSegments("order") {
        typedPath(OrderKey) { orderKey ⇒
          complete(orderKey.toString)
        }
      }

    "empty" in {
      Get("/prefix") ~> route ~> check {
        assert(!handled)
      }
    }

    "JobPath" in {
      Get("/job") ~> route ~> check {
        assert(!handled)
      }
      Get("/job/") ~> route ~> check {
        assert(!handled)
      }
      Get("/job/a") ~> route ~> check {
        assert(responseAs[String] == "Job:/a")
      }
      Get("/job/a,1") ~> route ~> check {
        assert(handled)
      }
      Get("/job/a/") ~> route ~> check {
        assert(!handled)
      }
      Get("/job/a/b") ~> route ~> check {
        assert(responseAs[String] == "Job:/a/b")
      }
    }

    "FolderPath" in {
      Get("/folder") ~> route ~> check {
        assert(!handled)
      }
      Get("/folder/") ~> route ~> check {
        assert(responseAs[String] == "Folder:/")
      }
      Get("/folder/a") ~> route ~> check {
        assert(!handled)
      }
      Get("/folder/a,1") ~> route ~> check {
        assert(!handled)
      }
      Get("/folder/a/") ~> route ~> check {
        assert(responseAs[String] == "Folder:/a")
      }
      Get("/folder/a/b/") ~> route ~> check {
        assert(responseAs[String] == "Folder:/a/b")
      }
    }

    "OrderKey" in {
      Get("/order") ~> route ~> check {
        assert(!handled)
      }
      Get("/order/") ~> route ~> check {
        assert(!handled)
      }
      Get("/order/a") ~> route ~> check {
        assert(!handled)
      }
      Get("/order/a,1") ~> route ~> check {
        assert(responseAs[String] == "Order:/a,1")
      }
      Get("/order/a/") ~> route ~> check {
        assert(!handled)
      }
      Get("/order/a/b/") ~> route ~> check {
        assert(!handled)
      }
    }
  }
}
