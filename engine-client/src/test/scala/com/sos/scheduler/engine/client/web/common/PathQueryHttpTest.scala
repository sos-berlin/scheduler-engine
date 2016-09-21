package com.sos.scheduler.engine.client.web.common

import com.sos.scheduler.engine.client.web.common.PathQueryHttp._
import com.sos.scheduler.engine.data.filebased.TypedPath
import com.sos.scheduler.engine.data.folder.FolderPath
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.queries.PathQuery
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class PathQueryHttpTest extends FreeSpec {

  "toUriPath, fromUriPath" in {
    intercept[IllegalArgumentException] { fromUriPath[FolderPath](Uri.Path.Empty) }
    intercept[IllegalArgumentException] { fromUriPath[JobPath](Uri.Path.Empty) }
    check[FolderPath]("/", PathQuery.All)
    check[JobPath   ]("/", PathQuery.All)
    check[FolderPath]("/", PathQuery(FolderPath("/"), isRecursive = true))
    check[JobPath   ]("/", PathQuery(FolderPath("/"), isRecursive = true))
    check[FolderPath]("/a/", PathQuery(FolderPath("/a"), isRecursive = true))
    check[JobPath   ]("/a/", PathQuery(FolderPath("/a"), isRecursive = true))
    check[FolderPath]("/a/*", PathQuery(FolderPath("/a"), isRecursive = false))
    check[JobPath   ]("/a/*", PathQuery(FolderPath("/a"), isRecursive = false))
    check[JobPath   ]("/a", PathQuery(JobPath("/a")))
    check[FolderPath]("/a", PathQuery(JobPath("/a")))
    intercept[IllegalArgumentException] { fromUriPath[JobChainPath](Uri.Path("/a,1")) }
  }

  private def check[P <: TypedPath: TypedPath.Companion](pathString: String, query: PathQuery): Unit = {
    val uriPath = Uri.Path(pathString)
    assert(fromUriPath[P](uriPath) == query)
    assert(toUriPath(query) == pathString)
  }
}
