package com.sos.scheduler.engine.client.web.common

import com.sos.scheduler.engine.data.filebased.TypedPath
import com.sos.scheduler.engine.data.queries.PathQuery
import shapeless.{::, HNil}
import spray.http.Uri
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
object PathQueryHttp {

  object directives {

    def pathQuery[P <: TypedPath](implicit companion: TypedPath.Companion[P]): Directive1[PathQuery] =
      new Directive1[PathQuery] {
        override def happly(inner: (PathQuery :: HNil) ⇒ Route) =
          unmatchedPath { path ⇒
            if (path startsWith Uri.Path.SingleSlash)
              handleExceptions(QueryHttp.PathExceptionHandler) {
                inner(fromUriPath[P](path) :: HNil)
              }
            else
              reject
          }
      }
  }

  private[common] def fromUriPath[P <: TypedPath: TypedPath.Companion](path: Uri.Path): PathQuery =
    PathQuery[P](path.toString)

  def toUriPath(query: PathQuery): String = query.toUriPath
}
