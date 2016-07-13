package com.sos.scheduler.engine.client.web.jobchain

import com.sos.scheduler.engine.data.jobchain.{JobChainPath, JobChainQuery}
import spray.http.Uri
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
object JobChainQueryHttp {

  object directives {
    def jobChainQuery: Directive1[JobChainQuery] =
      unmatchedPath flatMap { path ⇒
        if (path startsWith Uri.Path.SingleSlash)
          provide(fromUriPath(path.toString))
        else
          reject
      }
  }

  def fromUriPath(path: String) = new JobChainQuery(path)

  def toUriPath(query: JobChainQuery) = query.string

  def toUriPath(jobChainPath: JobChainPath) = jobChainPath.string
}
