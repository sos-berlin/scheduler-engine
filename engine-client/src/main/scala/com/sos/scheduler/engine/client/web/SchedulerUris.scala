package com.sos.scheduler.engine.client.web

import com.sos.scheduler.engine.client.web.SchedulerUris._
import com.sos.scheduler.engine.client.web.jobchain.JobChainQueryHttp
import com.sos.scheduler.engine.client.web.order.OrderQueryHttp
import com.sos.scheduler.engine.common.scalautil.Collections._
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, JobChainQuery}
import com.sos.scheduler.engine.data.order.OrderQuery
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
final class SchedulerUris private(schedulerUriString: String) {

  private val rootUri = Uri(s"$schedulerUriString/$Prefix/")

  lazy val command = resolvePathUri("api/command")

  lazy val overview = uriString("api")

  object order {
    def overviews(query: OrderQuery = OrderQuery.All): String =
      resolveWithOrderQuery(query, "OrderOverview").toString

    def fullOverview(query: OrderQuery = OrderQuery.All): String =
      resolveWithOrderQuery(query, "OrdersFullOverview").toString

    private def resolveWithOrderQuery(orderQuery: OrderQuery, typeName: String) = {
      val subpath = JobChainQueryHttp.toUriPath(orderQuery.jobChainQuery)
      resolvePathUri(Uri(
        path = Uri.Path(s"api/order$subpath"),
        query = Uri.Query(OrderQueryHttp.toHttpQueryMap(orderQuery) + ("return" → typeName))))
    }
  }

  object jobChain {
    def overview(jobChainPath: JobChainPath): String = overviews(JobChainQuery(jobChainPath))

    def overviews(query: JobChainQuery = JobChainQuery.All): String = {
      val subpath = JobChainQueryHttp.toUriPath(query)
      assert(subpath endsWith "/")
      uriString(Uri(path = Uri.Path(s"api/jobChain$subpath")))  // Default with trailing slash: query = Uri.Query("return" → "JobChainOverview")))
    }

    def details(jobChainPath: JobChainPath): String = {
      val subpath = JobChainQueryHttp.toUriPath(jobChainPath)
      assert(!subpath.endsWith("/"))
      uriString(Uri(path = Uri.Path(s"api/jobChain$subpath")))  // Default without trailing slash: query = Uri.Query("return" → "JobChainDetails")))
    }
  }

  /**
    * Public for tests only.
    */
  def uriString(uri: Uri): String = resolvePathUri(uri).toString

  def resolvePathUri(uri: Uri): Uri = resolveUri(uri, rootUri)

  override def toString = s"SchedulerUris($schedulerUriString)"
}

object SchedulerUris {
  private val Prefix = "jobscheduler/master"

  def apply(schedulerUri: Uri): SchedulerUris = apply(schedulerUri.toString)

  def apply(schedulerUri: String) = new SchedulerUris(schedulerUri stripSuffix "/")

  private[web] def resolveUri(uri: Uri, against: Uri) = {
    val scheme = emptyToNone(uri.scheme) getOrElse against.scheme
    val dummyAbsolute = Uri("dummy:")
    uri.resolvedAgainst(against resolvedAgainst dummyAbsolute).withScheme(scheme)
  }
}
