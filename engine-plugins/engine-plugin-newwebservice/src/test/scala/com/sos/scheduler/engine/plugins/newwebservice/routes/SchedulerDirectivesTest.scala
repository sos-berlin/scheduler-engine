package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.data.folder.FolderPath
import com.sos.scheduler.engine.data.job.JobPath
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
      pathPrefix("job") {
        typedPath(JobPath) { path ⇒
          complete(path.toString)
        }
      } ~
      pathPrefix("folder") {
        typedPath(FolderPath) { path ⇒
          complete(path.toString)
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
        assert(responseAs[String] == "Job /a")
      }
      Get("/job/a/") ~> route ~> check {
        assert(!handled)
      }
      Get("/job/a/b") ~> route ~> check {
        assert(responseAs[String] == "Job /a/b")
      }
    }

    "FolderPath" in {
      Get("/folder") ~> route ~> check {
        assert(!handled)
      }
      Get("/folder/") ~> route ~> check {
        assert(responseAs[String] == "Folder /")
      }
      Get("/folder/a") ~> route ~> check {
        assert(!handled)
      }
      Get("/folder/a/") ~> route ~> check {
        assert(responseAs[String] == "Folder /a")
      }
      Get("/folder/a/b/") ~> route ~> check {
        assert(responseAs[String] == "Folder /a/b")
      }
    }
  }
}
