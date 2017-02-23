package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.jobscheduler.data.filebased.TypedPath
import com.sos.jobscheduler.data.folder.FolderPath
import scala.util.{Failure, Success, Try}
import shapeless.{::, HNil}
import spray.http.Uri
import spray.routing.Directives._
import spray.routing.{Directive1, Route}

/**
  * @author Joacim Zschimmer
  */
object SchedulerDirectives {

  def typedPath[A <: TypedPath: TypedPath.Companion]: Directive1[A] =
    new Directive1[A] {
      def happly(inner: (A :: HNil) ⇒ Route) =
        unmatchedPath {
          case path: Uri.Path.Slash ⇒
            val toPath = implicitly[TypedPath.Companion[A]]
            Try(toPath(normalizePath(toPath, path.toString))) match {
              case Failure(t) ⇒ reject // Without message because the whole route is wrong and the next route should be tried (otherwise 404)
              case Success(aPath) ⇒
                mapRequestContext(_.copy(unmatchedPath = Uri.Path.Empty)) {
                  inner(aPath :: HNil)
                }
            }
          case _ ⇒ reject
        }
    }

  private def normalizePath(companion: TypedPath.AnyCompanion, path: String) = {
    if (companion == FolderPath) {
      require(path endsWith "/", "Missing trailing slash")
      if (path == "/")
        path
      else
        path stripSuffix "/"
    } else
      path
  }
}
