package com.sos.scheduler.engine.client.web.jobchain

import com.sos.scheduler.engine.client.web.jobchain.PathQueryHttp._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.queries.PathQuery
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class PathQueryHttpTest extends FreeSpec {

  "toUriPath" in {
    assert(toUriPath(PathQuery.All) == "/")
    assert(toUriPath(PathQuery(JobPath("/a/b"))) == "/a/b")
  }
}
