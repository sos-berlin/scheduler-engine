package com.sos.scheduler.engine.client.web.common

import com.sos.jobscheduler.base.serial.PathAndParameterSerializable
import com.sos.jobscheduler.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.jobscheduler.common.sprayutils.SprayUtils._
import com.sos.jobscheduler.data.filebased.TypedPath
import com.sos.scheduler.engine.cplusplus.runtime.CppException
import com.sos.scheduler.engine.data.queries.{JobChainNodeQuery, JobChainQuery, OrderQuery, PathQuery}
import scala.util.{Failure, Success, Try}
import shapeless.{::, HNil}
import spray.http.StatusCodes.BadRequest
import spray.json.RootJsonFormat
import spray.routing.Directives._
import spray.routing._
import spray.routing.directives.MarshallingDirectives

/**
  * @author Joacim Zschimmer
  */
object QueryHttp {

  def jobChainQuery = getOrPost[JobChainQuery]

  def jobChainNodeQuery = getOrPost[JobChainNodeQuery]

  def orderQuery = getOrPost[OrderQuery]

  def pathQuery[P <: TypedPath: TypedPath.Companion]: Directive[PathQuery :: HNil] = {
    implicit val x = PathQuery.jsonFormat[P]
    implicit val y = PathQuery.pathAndParameterSerializable[P]
    getOrPost[PathQuery]
  }

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

  private def pathAndParametersDirective[A: PathAndParameterSerializable] =
    new Directive1[A] {
      def happly(inner: (A :: HNil) ⇒ Route) =
        unmatchedPath { path ⇒
          parameterMap { parameters ⇒
            Try(implicitly[PathAndParameterSerializable[A]].fromPathAndParameters((path.toString, parameters))) match {
              case Failure(t: IllegalArgumentException) ⇒ reject(ValidationRejection(Option(t.getMessage) getOrElse "Query not recognized", Some(t)))
              case Failure(t) ⇒ throw t
              case Success(result) ⇒
                handleExceptions(PathExceptionHandler) {
                  inner(result :: HNil)
                }
            }
          }
        }
    }

  private[common] val PathExceptionHandler = ExceptionHandler {
    case e: CppException if e.getCode == "SCHEDULER-161" ⇒
      completeWithError(BadRequest, e.getMessage)
    case e: NoSuchElementException if e.getMessage startsWith "SCHEDULER-161" + " " ⇒
      completeWithError(BadRequest, e.getMessage)
  }
}
