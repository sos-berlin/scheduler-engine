package com.sos.scheduler.engine.client.web.order

import com.google.common.base.Splitter
import com.sos.scheduler.engine.client.web.jobchain.{PathQueryHttp, QueryHttp}
import com.sos.scheduler.engine.common.convert.As
import com.sos.scheduler.engine.common.convert.ConvertiblePartialFunctions._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderId, OrderSourceType}
import com.sos.scheduler.engine.data.queries.{OrderIdQuery, OrderQuery}
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
      jobChainPathQuery = PathQueryHttp.fromUriPath[JobChainPath](path),
      orderIdQuery = OrderIdQuery(parameters.optionAs[String](OrderIdName) map OrderId.apply),
      isDistributed = parameters.optionAs[Boolean](IsDistributedName),
      isSuspended = parameters.optionAs[Boolean](IsSuspendedName),
      isSetback = parameters.optionAs[Boolean](IsSetbackName),
      isBlacklisted = parameters.optionAs[Boolean](IsBlacklistedName),
      isOrderSourceType = parameters.optionAs(IsOrderSourceTypeName)(As { o ⇒ (CommaSplitter.split(o) map OrderSourceType.valueOf).toSet }),
      notInTaskLimitPerNode = parameters.optionAs[Int](NotInTaskLimitPerNode))

  def toUriPath(q: OrderQuery): String = q.jobChainPathQuery.patternString
}
