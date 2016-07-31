package com.sos.scheduler.engine.client.web.jobchain

import com.sos.scheduler.engine.data.queries.{JobChainQuery, PathQuery}
import spray.http.Uri.Path
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
object JobChainQueryHttp {

  private val DistributedName = "distributed"
  protected val parameterNames = Set(DistributedName)

  object directives {
    def orderQuery: Directive1[JobChainQuery] = QueryHttp.pathAndParametersDirective(pathAndParametersToQuery)
  }

  protected def pathAndParametersToQuery(path: Path, parameters: Map[String, String]) =
    JobChainQuery.Standard(
      jobChainPathQuery = PathQuery(path.toString),
      isDistributed = parameters.get(DistributedName) map { _.toBoolean })

  def toUriPath(q: JobChainQuery) = q.jobChainPathQuery.string

  def toHttpQueryMap(q: JobChainQuery) = Map() ++
    (q.isDistributed map { o ⇒ DistributedName → o.toString })
}
