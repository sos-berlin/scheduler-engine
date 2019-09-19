package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.plugins.newwebservice.routes.LiveRoute.test.{allowedPaths, forbiddenPaths}
import com.sos.scheduler.engine.plugins.newwebservice.routes.LiveRoute.{isAllowedPath, rebuildPath}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import spray.http.Uri

@RunWith(classOf[JUnitRunner])
final class LiveRouteTest extends FreeSpec
{
  "rebuildPath" in {
    assert(rebuildPath(Uri.Path("")) == "")
    assert(rebuildPath(Uri.Path("/a\\/b/c")) == "/a\\/b/c")
    assert(rebuildPath(Uri.Path("a\\/b/c")) == "a\\/b/c")
    assert(rebuildPath(Uri.Path("/a%2Fb/c")) == "/a/b/c")
  }

  "Forbidden paths" - {
    for (path <- forbiddenPaths) path in {
      assert(!isAllowedPath(path))
    }
  }

  "Allowed paths" - {
    for (path <- allowedPaths) path in {
      assert(isAllowedPath(path))
    }
  }
}
