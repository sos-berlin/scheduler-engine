package com.sos.scheduler.engine.client.web.jobchain

import com.sos.scheduler.engine.client.web.common.{PathQueryHttp, QueryHttp}
import com.sos.scheduler.engine.common.convert.ConvertiblePartialFunctions._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.queries.JobChainQuery
import com.sos.scheduler.engine.data.queries.JobChainQuery.IsDistributedName
import spray.http.Uri
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
object JobChainQueryHttp {

  private val DistributedName = "isDistributed"
  protected val parameterNames = Set(DistributedName)

  object directives {
    def jobChainQuery: Directive1[JobChainQuery] = QueryHttp.pathAndParametersDirective(toJobChainQuery)
  }

  def toJobChainQuery(path: Uri.Path, parameters: Map[String, String]): JobChainQuery.Standard =
    JobChainQuery.Standard(
      jobChainPathQuery = PathQueryHttp.fromUriPath[JobChainPath](path),
      isDistributed = parameters.optionAs[Boolean](IsDistributedName))

  def toUriPath(q: JobChainQuery): String = q.jobChainPathQuery.toUriPath
}
