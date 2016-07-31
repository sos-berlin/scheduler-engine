package com.sos.scheduler.engine.client.web.jobchain

import com.sos.scheduler.engine.data.queries.PathQuery
import spray.http.Uri
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
object PathQueryHttp {

  object directives {
    def pathQuery: Directive1[PathQuery] =
      unmatchedPath flatMap { path ⇒
        if (path startsWith Uri.Path.SingleSlash)
          provide(fromUriPath(path))
        else
          reject
      }
  }

  def fromUriPath(path: Uri.Path) = new PathQuery(path.toString)

  def toUriPath(query: PathQuery) = query.string
}
