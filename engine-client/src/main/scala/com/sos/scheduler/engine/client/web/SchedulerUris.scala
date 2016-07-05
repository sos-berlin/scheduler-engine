package com.sos.scheduler.engine.client.web

import com.sos.scheduler.engine.client.web.SchedulerUris._
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
    lazy val overviews = uriString("api/order/OrderOverview/")
    lazy val fullOverview = uriString("api/order/OrdersFullOverview")
  }

  /**
    * Public for tests only.
    */
  def uriString(uri: Uri): String = resolvePathUri(uri).toString

  def resolvePathUri(uri: Uri): Uri = {
    val u = uri.resolvedAgainst(prefixedUri)
    u.copy(path = Path(s"${prefixedUri.path}/${stripLeadingSlash(uri.path.toString)}"))
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
