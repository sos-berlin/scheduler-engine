package com.sos.scheduler.engine.plugins.newwebservice.html

import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
final class WebServiceContext(
  val baseUri: Uri,
  val webjarsUri: Uri,
  val htmlEnabled: Boolean)
{
  def toStylesheetLinkHtml(relativePath: String) = <link rel="stylesheet" href={s"$webjarsUri/$relativePath"}/>
}
