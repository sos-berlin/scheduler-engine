package com.sos.scheduler.engine.client.web.order

import com.google.common.base.Splitter
import com.sos.scheduler.engine.client.web.jobchain.{PathQueryHttp, QueryHttp}
import com.sos.scheduler.engine.common.convert.As
import com.sos.scheduler.engine.common.convert.ConvertiblePartialFunctions._
import com.sos.scheduler.engine.data.order.OrderSourceType
import com.sos.scheduler.engine.data.queries.OrderQuery
import com.sos.scheduler.engine.data.queries.OrderQuery._
import scala.collection.JavaConversions._
import spray.http.Uri
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
object OrderQueryHttp {

  private val CommaSplitter = Splitter.on(',').omitEmptyStrings.trimResults

  object directives {
    def extendedOrderQuery: Directive1[OrderQuery] = QueryHttp.pathAndParametersDirective(pathAndParametersToQuery)
  }

  def pathAndParametersToQuery(path: Uri.Path, parameters: Map[String, String]): OrderQuery =
    OrderQuery(
      jobChainPathQuery = PathQueryHttp.fromUriPath(path),
      isDistributed = parameters.optionAs[Boolean](DistributedName),
      isSuspended = parameters.optionAs[Boolean](SuspendedName),
      isSetback = parameters.optionAs[Boolean](SetbackName),
      isBlacklisted = parameters.optionAs[Boolean](BlacklistedName),
      isOrderSourceType = parameters.optionAs(SourceTypeName)(As { o ⇒ (CommaSplitter.split(o) map OrderSourceType.valueOf).toSet }),
      limitPerNode = parameters.optionAs[Int](LimitPerNodeName))

  def toUriPath(q: OrderQuery): String = q.jobChainPathQuery.patternString
}
