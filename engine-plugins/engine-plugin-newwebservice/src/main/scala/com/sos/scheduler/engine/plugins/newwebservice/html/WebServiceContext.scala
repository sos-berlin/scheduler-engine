package com.sos.scheduler.engine.plugins.newwebservice.html

import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
final class WebServiceContext(
  val baseUri: Uri = Uri("/"),
  val webjarsUri: Uri = Uri("/jobscheduler/master/webjars"),
  val htmlEnabled: Boolean = false)
{
  def toStylesheetLinkHtml(relativePath: String) = <link rel="stylesheet" href={s"$webjarsUri/$relativePath"}/>
}
