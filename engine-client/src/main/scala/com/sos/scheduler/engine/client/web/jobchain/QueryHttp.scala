package com.sos.scheduler.engine.client.web.jobchain

import scala.util.{Failure, Success, Try}
import shapeless.{::, HNil}
import spray.http.Uri
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
object QueryHttp {

  def pathAndParametersDirective[A](pathAndParametersToA: (Uri.Path, Map[String, String]) ⇒ A): Directive1[A] =
    new Directive1[A] {
      def happly(inner: (A :: HNil) ⇒ Route) =
        unmatchedPath { path ⇒
          parameterMap { parameters ⇒
            Try(pathAndParametersToA(path, parameters)) match {
              case Failure(t: IllegalArgumentException) ⇒ reject(ValidationRejection(Option(t.getMessage) getOrElse "Query not recognized", Some(t)))
              case Failure(t) ⇒ throw t
              case Success(result) ⇒ inner(result :: HNil)
            }
          }
        }
    }
}
