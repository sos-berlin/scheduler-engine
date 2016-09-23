package com.sos.scheduler.engine.client.web.common

import com.sos.scheduler.engine.data.filebased.TypedPath
import com.sos.scheduler.engine.data.queries.PathQuery
import spray.http.Uri
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
object PathQueryHttp {

  object directives {

    def pathQuery[P <: TypedPath](implicit companion: TypedPath.Companion[P]): Directive1[PathQuery] =
      unmatchedPath flatMap { path ⇒
        if (path startsWith Uri.Path.SingleSlash)
          provide(fromUriPath[P](path))
        else
          reject
      }
  }

  private[common] def fromUriPath[P <: TypedPath: TypedPath.Companion](path: Uri.Path): PathQuery =
    PathQuery[P](path.toString)

  def toUriPath(query: PathQuery): String = query.toUriPath
}
