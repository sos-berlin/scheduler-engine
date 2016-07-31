package com.sos.scheduler.engine.client.web.order

import com.google.common.base.Splitter
import com.sos.scheduler.engine.client.web.jobchain.{PathQueryHttp, QueryHttp}
import com.sos.scheduler.engine.data.order.OrderSourceType
import com.sos.scheduler.engine.data.queries.OrderQuery
import scala.collection.JavaConversions._
import spray.http.Uri
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
  private val DistributedName = "distributed"

  private val CommaSplitter = Splitter.on(',').omitEmptyStrings.trimResults

  object directives {
    def extendedOrderQuery: Directive1[OrderQuery] = QueryHttp.pathAndParametersDirective(pathAndParametersToQuery)
  }

  def pathAndParametersToQuery(path: Uri.Path, parameters: Map[String, String]): OrderQuery =
    OrderQuery(
        jobChainPathQuery = PathQueryHttp.fromUriPath(path),
        isDistributed = parameters.get(DistributedName) map { _.toBoolean },
        isSuspended = parameters.get(SuspendedName) map { _.toBoolean },
        isSetback = parameters.get(SetbackName) map { _.toBoolean },
        isBlacklisted = parameters.get(BlacklistedName) map { _.toBoolean },
        isOrderSourceType = parameters.get(SourceTypeName) map { o ⇒ (CommaSplitter.split(o) map OrderSourceType.valueOf).toSet })

  def toUriPath(q: OrderQuery) = q.jobChainPathQuery.string

  def toUriQueryMap(q: OrderQuery): Map[String, String] = Map() ++
    (q.isSuspended map { o ⇒ SuspendedName → o.toString }) ++
    (q.isSetback map { o ⇒ SetbackName → o.toString }) ++
    (q.isBlacklisted map { o ⇒ BlacklistedName → o.toString}) ++
    (q.isOrderSourceType map ( o ⇒ SourceTypeName → (o mkString ","))) ++
    (q.isDistributed map { o ⇒ DistributedName → o.toString })
}
