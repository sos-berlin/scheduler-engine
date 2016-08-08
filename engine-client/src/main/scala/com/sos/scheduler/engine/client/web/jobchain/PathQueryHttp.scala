package com.sos.scheduler.engine.client.web.jobchain

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
    def pathQuery[A <: TypedPath](implicit companion: TypedPath.Companion[A]): Directive1[PathQuery] =
      unmatchedPath flatMap { path ⇒
        if (path startsWith Uri.Path.SingleSlash)
          provide(fromUriPath[A](path))
        else
          reject
      }
  }

  def fromUriPath[A <: TypedPath: TypedPath.Companion](path: Uri.Path): PathQuery = {
    val pathString = path.toString
    //implicitly[TypedPath.Companion[A]].apply(pathString)  // Check path name
    PathQuery(pathString)
  }

  def toUriPath(query: PathQuery): String = query.patternString
}
