package com.sos.scheduler.engine.client.web.common

import com.sos.scheduler.engine.base.serial.PathAndParameterSerializable
import com.sos.scheduler.engine.data.queries.{JobChainQuery, OrderQuery}
import scala.util.{Failure, Success, Try}
import shapeless.{::, HNil}
import spray.json.RootJsonFormat
import spray.routing.Directives._
import spray.routing._
import spray.routing.directives.MarshallingDirectives
import spray.httpx.SprayJsonSupport._

/**
  * @author Joacim Zschimmer
  */
object QueryHttp {

  def jobChainQuery = getOrPost[JobChainQuery]

  def orderQuery = getOrPost[OrderQuery]

  def getOrPost[A: RootJsonFormat: PathAndParameterSerializable] =
    new Directive1[A] {
      def happly(inner: (A :: HNil) ⇒ Route) =
        get {
          val directive = pathAndParametersDirective[A]
          directive { query ⇒
            inner(query :: HNil)
          }
        } ~
        (post & pathEnd) {
          entity(MarshallingDirectives.as[A]) { query ⇒
            inner(query :: HNil)
          }
        }
    }

  def pathAndParametersDirective[A: PathAndParameterSerializable] =
    new Directive1[A] {
      def happly(inner: (A :: HNil) ⇒ Route) =
        unmatchedPath { path ⇒
          parameterMap { parameters ⇒
            Try(implicitly[PathAndParameterSerializable[A]].fromPathAndParameters((path.toString, parameters))) match {
              case Failure(t: IllegalArgumentException) ⇒ reject(ValidationRejection(Option(t.getMessage) getOrElse "Query not recognized", Some(t)))
              case Failure(t) ⇒ throw t
              case Success(result) ⇒ inner(result :: HNil)
            }
          }
        }
    }
}
