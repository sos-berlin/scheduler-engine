package com.sos.scheduler.engine.client.web.jobchain

import com.sos.scheduler.engine.client.web.jobchain.QueryHttp._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.http.Uri
import spray.routing.Directives._
import spray.testkit.ScalatestRouteTest

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class QueryHttpTest extends FreeSpec with ScalatestRouteTest {

  "pathAndParametersDirective" - {
    def pathAndParametersToString(path: Uri.Path, parameters: Map[String, String]) =
      Vector(path.toString) ++ parameters.get("x") ++ parameters.get("y")
    val route =
      pathPrefix("prefix") {
        pathAndParametersDirective(pathAndParametersToString) { strings: Vector[String] ⇒
          complete(strings mkString ",")
        }
      }

    "empty" in {
      Get("/prefix") ~> route ~> check {
        assert(responseAs[String] == "")
      }
    }

    "non-empty" in {
      Get("/prefix/") ~> route ~> check {
        assert(responseAs[String] == "/")
      }
      Get("/prefix/a/b") ~> route ~> check {
        assert(responseAs[String] == "/a/b")
      }
      Get("/prefix/a/b?x=1&y=2") ~> route ~> check {
        assert(responseAs[String] == "/a/b,1,2")
      }
    }
  }
}
