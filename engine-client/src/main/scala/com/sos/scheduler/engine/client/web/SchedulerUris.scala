package com.sos.scheduler.engine.client.web

import com.sos.scheduler.engine.client.web.SchedulerUris._
import com.sos.scheduler.engine.client.web.order.OrderQueryHttp
import com.sos.scheduler.engine.data.order.OrderQuery
import spray.http.Uri
import spray.http.Uri.Path

/**
  * @author Joacim Zschimmer
  */
final class SchedulerUris private(schedulerUriString: String) {

  private val prefixedUri = Uri(s"$schedulerUriString/$Prefix")

  lazy val command = resolvePathUri("api/command")

  lazy val overview = uriString("api")

  object order {
    def overviews(query: OrderQuery = OrderQuery.All): String =
      resolveWithOrderQuery(query, "OrderOverview").toString

    def fullOverview(query: OrderQuery = OrderQuery.All): String =
      resolveWithOrderQuery(query, "OrdersFullOverview").toString

    private def resolveWithOrderQuery(orderQuery: OrderQuery, typeName: String) =
      resolvePathUri(Uri(
        path = Uri.Path(s"api/order${orderQuery.jobChainQuery.uriPath}"),
        query = Uri.Query(OrderQueryHttp.toHttpQueryMap(orderQuery) + ("return" → typeName))))
  }

  /**
    * Public for tests only.
    */
  def uriString(uri: Uri): String = resolvePathUri(uri).toString

  def resolvePathUri(uri: Uri): Uri = {
    val u = uri.resolvedAgainst(prefixedUri)
    u.copy(path = Path(s"${prefixedUri.path}/${stripLeadingSlash(uri.path.toString)}"), query = u.query)
  }

  override def toString = s"SchedulerUris($schedulerUriString)"
}

object SchedulerUris {
  private val Prefix = "jobscheduler/master"

  def apply(schedulerUri: Uri): SchedulerUris = apply(schedulerUri.toString)

  def apply(schedulerUri: String) = new SchedulerUris(schedulerUri stripSuffix "/")

  private def stripLeadingSlash(o: String) =
    o match {
      case _ ⇒ o stripPrefix "/"
    }
}
