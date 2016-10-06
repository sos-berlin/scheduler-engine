package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.client.web.SchedulerUris
import scala.collection.immutable
import scalatags.Text.all._
import spray.http.Uri
import HtmlIncluder._
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlPage.seqFrag

/**
  * @author Joacim Zschimmer
  */
final class HtmlIncluder(uris: SchedulerUris) {

  def webjarsToHtml(webjar: Webjar): immutable.Seq[Frag] =
    (webjar.cssPaths map toWebjarUri map toCssLinkHtml) ++
      (webjar.javascriptPaths map toWebjarUri map toScriptHtml) ++
      (for (initialize ← webjar.initialize) yield script(`type` := "text/javascript")(s"jQuery(function() { $initialize })"))

  def cssHtml(webjar: Webjar): Frag =
    webjar.cssPaths map toWebjarUri map toCssLinkHtml

  def javascriptHtml(webjar: Webjar): Frag =
    (webjar.javascriptPaths map toWebjarUri map toScriptHtml) ++
      (for (initialize ← webjar.initialize) yield script(`type` := "text/javascript")(s"jQuery(function() { $initialize })"))

  private def toWebjarUri(path: String) = uris / s"api/frontend/webjars/$path"
}

object HtmlIncluder {

  def toCssLinkHtml(uri: Uri) = link(rel := "stylesheet", `type` := "text/css", href := uri.toString)

  def toScriptHtml(uri: Uri) = script(`type` := "text/javascript", src := uri.toString)

  def toAsyncScriptHtml(uri: Uri) = script(`type` := "text/javascript", src := uri.toString, "async".emptyAttr)
}
