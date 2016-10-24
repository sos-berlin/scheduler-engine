package com.sos.scheduler.engine.scalajs.plugin

import com.sos.scheduler.engine.client.web.SchedulerUris
import scalatags.Text.all._

/**
  * @author Joacim Zschimmer
  */
final class SingleScalaJsPage extends ScalaJsPage {

  protected val uris: SchedulerUris = SchedulerUris("/")

  override protected def scriptLinks = super.scriptLinks :+ uris / "scalajs/run.js"

  def wholePage = htmlPage(
    div(id := "APP"),
    raw("<script type='text/javascript'>" +
      "document.getElementById('APP').appendChild(com.sos.scheduler.engine.scalajs.frontend.Run().run());" +
      "</script>"))
}
