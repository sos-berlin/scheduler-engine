package com.sos.scheduler.engine.client.web

import com.sos.scheduler.engine.client.web.SchedulerUris._
import spray.http.Uri
import spray.http.Uri.Path

/**
  * @author Joacim Zschimmer
  */
final class SchedulerUris private(schedulerUriString: String) {

  private val prefixedUri = Uri(s"$schedulerUriString/$Prefix")

  val overview = uriString(Api)

  object order {
    val overviews = uriString(s"$Api/order/OrderOverview/")
  }

  object test {
    val error500 = uriString(s"$Api/test/ERROR-500")
    val outOfMemoryError = uriString(s"$Api/test/OutOfMemoryError")
    val unknown = uriString(s"$Api/test/UNKNOWN")
  }

  private def uriString(uri: Uri): String = resolvePathUri(uri).toString

  def resolvePathUri(uri: Uri): Uri = {
    val u = uri.resolvedAgainst(prefixedUri)
    u.copy(path = Path(s"${prefixedUri.path}/${stripLeadingSlash(uri.path.toString)}"))
  }
}

object SchedulerUris {
  private val Prefix = "new/master"
  private val Api = "api"

  def apply(schedulerUri: String) = new SchedulerUris(schedulerUri stripSuffix "/")

  private def stripLeadingSlash(o: String) =
    o match {
      case _ ⇒ o stripPrefix "/"
    }
}
