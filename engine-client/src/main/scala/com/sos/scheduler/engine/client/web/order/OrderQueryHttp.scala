package com.sos.scheduler.engine.client.web.order

import com.google.common.base.Splitter
import com.sos.scheduler.engine.data.jobchain.JobChainQuery
import com.sos.scheduler.engine.data.order.{OrderQuery, OrderSourceType}
import scala.collection.JavaConversions._
import spray.http.Uri
import spray.routing.Directives._
import spray.routing._
import spray.routing.directives.BasicDirectives.{extract ⇒ _, provide ⇒ _}

/**
  * @author Joacim Zschimmer
  */
object OrderQueryHttp {

  private val SuspendedName = "suspended"
  private val SetbackName = "setback"
  private val BlacklistedName = "blacklisted"
  private val SourceTypeName = "sourceType"
  val Names = Set(SuspendedName, SetbackName, BlacklistedName, SourceTypeName)

  private val CommaSplitter = Splitter.on(',').omitEmptyStrings.trimResults

  def fromHttpQuery(parameters: Map[String, String]): Either[Rejection, OrderQuery] = {
    val unknown = parameters.keySet -- Names
      try {
        require(unknown.isEmpty, s"Unknown parameter for OrderQuery: ${unknown mkString ", "}")
        Right(new OrderQuery(
          isSuspended = parameters.get(SuspendedName) map { _.toBoolean },
          isSetback = parameters.get(SetbackName) map { _.toBoolean },
          isBlacklisted = parameters.get(BlacklistedName) map { _.toBoolean },
          isSourceType = parameters.get(SourceTypeName) map { o ⇒ (CommaSplitter.split(o) map OrderSourceType.valueOf).toSet }))
      } catch {
        case e: IllegalArgumentException ⇒
          Left(ValidationRejection(Option(e.getMessage) getOrElse "Error in OrderQuery", Some(e)))
      }
  }

  def toHttpQueryMap(q: OrderQuery): Map[String, String] = Map() ++
    (q.isSuspended map { o ⇒ SuspendedName → o.toString }) ++
    (q.isSetback map { o ⇒ SetbackName → o.toString }) ++
    (q.isBlacklisted map { o ⇒ BlacklistedName → o.toString}) ++
    (q.isSourceType map ( o ⇒ SourceTypeName → (o mkString ",")))

  object directives {
    def orderQuery(parameters: Map[String, String]): Directive1[OrderQuery] =
      unmatchedPath flatMap { path ⇒
        if (path startsWith Uri.Path.SingleSlash)
          OrderQueryHttp.fromHttpQuery(parameters) match {
            case Right(query) ⇒ provide(query.copy(jobChainQuery = JobChainQuery.fromUriPath(path.toString)))
            case Left(rejection) ⇒ reject(rejection)
          }
        else
          reject
      }
    }
}
