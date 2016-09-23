package com.sos.scheduler.engine.client.web.order

import com.google.common.base.Splitter
import com.sos.scheduler.engine.client.web.common.QueryHttp
import com.sos.scheduler.engine.client.web.jobchain.JobChainQueryHttp.toJobChainQuery
import com.sos.scheduler.engine.common.convert.As
import com.sos.scheduler.engine.common.convert.ConvertiblePartialFunctions._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.order.{OrderId, OrderProcessingState, OrderSourceType}
import com.sos.scheduler.engine.data.queries.OrderQuery._
import com.sos.scheduler.engine.data.queries.{JobChainQuery, OrderQuery}
import scala.collection.JavaConversions._
import spray.http.Uri
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
object OrderQueryHttp {

  private val CommaSplitter = Splitter.on(',')

  private[order] def commaSplittedAsSet[A](to: String ⇒ A) = As[String, Set[A]] {
    case "" ⇒ Set()
    case o ⇒ (CommaSplitter split o map to).toSet
  }

  object directives {
    def orderQuery: Directive1[OrderQuery] = QueryHttp.pathAndParametersDirective(toOrderQuery)
  }

  private[order] def toOrderQuery(path: Uri.Path, parameters: Map[String, String]): OrderQuery =
    OrderQuery(
      jobChainQuery = toJobChainQuery(path, parameters),
      orderIds = parameters.optionAs(OrderIdsName)(commaSplittedAsSet(OrderId.apply)),
      jobPaths = parameters.optionAs(JobPathsName)(commaSplittedAsSet(JobPath.apply)),
      isSuspended = parameters.optionAs[Boolean](IsSuspendedName),
      isSetback = parameters.optionAs[Boolean](IsSetbackName),
      isBlacklisted = parameters.optionAs[Boolean](IsBlacklistedName),
      isOrderSourceType = parameters.optionAs(IsOrderSourceTypeName)(commaSplittedAsSet(OrderSourceType.valueOf)),
      isOrderProcessingState = parameters.optionAs(IsOrderProcessingStateName)(commaSplittedAsSet(OrderProcessingState.typedJsonFormat.typeNameToClass)),
      notInTaskLimitPerNode = parameters.optionAs[Int](NotInTaskLimitPerNode),
      orIsSuspended = parameters.as[Boolean](OrIsSuspendedName, false))

  def toUriPath(q: OrderQuery): String = q.jobChainQuery.pathQuery.toUriPath
}
