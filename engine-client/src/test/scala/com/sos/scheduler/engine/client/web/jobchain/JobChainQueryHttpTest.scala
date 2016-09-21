package com.sos.scheduler.engine.client.web.jobchain

import com.sos.scheduler.engine.client.web.jobchain.JobChainQueryHttp.directives.jobChainQuery
import com.sos.scheduler.engine.data.folder.FolderPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.queries.{JobChainQuery, PathQuery}
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
final class JobChainQueryHttpTest extends FreeSpec with ScalatestRouteTest {

  "JobChainQuery" - {
    def route(expected: ⇒ JobChainQuery) =
      pathPrefix("prefix") {
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
      Get("/prefix/a/b") ~> route(JobChainQuery(jobChainPathQuery = PathQuery[JobChainPath]("/a/b"))) ~> check {
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

  private def checkQuery(jobChainQuery: JobChainQuery, path: Uri.Path, parameters: Map[String, String]) = {
    assert(jobChainQuery.toUriPathAndParameters == ((path.toString, parameters)))
    assert(JobChainQueryHttp.toJobChainQuery(path, Map(parameters.toVector: _*)) == jobChainQuery)
  }
}
