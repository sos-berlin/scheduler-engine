package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.jobscheduler.common.scalautil.AutoClosing.autoClosing
import com.sos.jobscheduler.common.utils.JavaResource
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.HtmlIncluder._
import java.math.BigInteger
import java.net.URL
import java.security.MessageDigest
import scala.collection.immutable
import scalatags.Text.all._
import spray.http.Uri

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

private[simplegui] object HtmlIncluder {
  private val StaticPackage = JavaResource("com/sos/scheduler/engine/plugins/newwebservice/simplegui/frontend/")

  def toCssLinkHtml(uri: Uri) = link(rel := "stylesheet", `type` := "text/css", href := uri.toString)

  def toScriptHtml(uri: Uri) = script(`type` := "text/javascript", src := uri.toString)

  def toVersionedUriPath(resource: JavaResource): String = {
    require(resource.path startsWith StaticPackage.path)
    val path = resource.path.stripPrefix(StaticPackage.path)
    val hash = uriToSha224(resource.url)
    s"api/frontend/$path?SHA-224=$hash"
  }

  //def toAsyncScriptHtml(uri: Uri) = script(`type` := "text/javascript", src := uri.toString, "async".emptyAttr)

  private[simplegui] def uriToSha224(url: URL): String = {
    val messageDigest = MessageDigest.getInstance("SHA-224")
    autoClosing(url.openStream()) { in ⇒
      val buffer = new Array[Byte](4096)
      var end = false
      while (!end) {
        val len = in.read(buffer)
        if (len > 1) {
          messageDigest.update(buffer, 0, len)
        } else {
          end = true
        }
      }
    }
    toHexString(messageDigest.digest)
  }

  private def toHexString(bytes: Array[Byte]): String =
    new BigInteger(+1, bytes) formatted s"%0${bytes.length}x"
}
