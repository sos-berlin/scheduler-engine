package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.scheduler.engine.data.event.Snapshot
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.ToHtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.SchedulerHtmlPage.instantWithDurationToHtml
import scala.concurrent.Future
import scalatags.Text.all._
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
final class SchedulerOverviewHtmlPage private(
  protected val snapshot: Snapshot[SchedulerOverview],
  protected val pageUri: Uri,
  protected val uris: SchedulerUris)
extends SchedulerHtmlPage {

  protected val schedulerOverview = snapshot.value
  import schedulerOverview._

  override protected def cssLinks = super.cssLinks :+ uris / "api/frontend/schedulerOverview/overview.css"

  def wholePage =
    htmlPage(
      systemInformationHtml,
      p(marginBottom := "30px")(
        s"Started at ",
        instantWithDurationToHtml(startedAt),
        " · ",
        ( (httpPort map { o ⇒ s" HTTP port $o" }) ++
          (udpPort map { o ⇒ s" UDP port $o" }) ++
          Some(s"PID $pid") ++
          Some(state))
          .mkString(" · ")),
      form(action := "api/command", method := "get")(
        span(cls := "input-group input-group-sm")(
          span(cls := "input-group-addon")(
            "XML command: "),
          input(`type` := "text", autofocus, name := "command", value := "s", placeholder := "For example: show_state", cls := "form-control"),
          " ",
          span(cls := "input-group-btn")
            (button(`type` := "submit", cls := "btn btn-primary")(
              "Execute")))))

//  private def cpuAndRamGraphics =
//    div(width := 170.px)(
//      "progressbar".tag[String](cls := "progress-bar-info", "max".attr := 1, "animate".attr := false, margin := 0, fontSize := 11.px)(
//        system.mxBeans.get("operatingSystem") flatMap { _.asInstanceOf[Map[String, Any]].get("systemCpuLoad") } getOrElse "")
//        //span(color.white, whiteSpace.nowrap)(system.mxBeans.operatingSystem.systemCpuLoad | percentage:0 "CPU"),
//      "progress".tag[String]("max".attr := 1, margin := "5px 0 0 0", fontSize := 11.px, "animate".attr := false)(
//        "bar".tag[String](1 - freePhysicalMemoryRatio)(
//          span(color.white)((totalPhysicalMemorySize - freePhysicalMemorySize) / (1024*1024*1024) | number:1 "GiB")()
//        bar(`type` := 'info')(freePhysicalMemoryRatio)(
//          span(color.black)((freePhysicalMemorySize / (1024*1024*1024) | number:1) "GiB"))),
//      div(cls := "smallFont", margin := 0, color := 0xa0a0a0)(
//        (totalPhysicalMemorySize / (1024*1024*1024) | number:1 " GiB RAM ∙ "),
//        span(color := 0x08000)(freePhysicalMemorySize / (1024*1024*1024) | number:1 "GiB free")

  private def systemInformationHtml =
    div(cls := "ContentBox SystemInformation")(
//      div(float.right, marginTop := 1.px, marginLeft := 10.px)(
//        cpuAndRamGraphics),
      div(/*float.right*/)(
        div(cls := "Hostname")(
          system.hostname),
        for (o ← system.distribution) yield
          div(o),
        div(
          java.systemProperties.getOrElse("os.name", "") + " " +
            java.systemProperties.getOrElse("os.version", "") + " ∙ " +
            java.systemProperties.getOrElse("os.arch", "")),
        div(
          "Java " + java.systemProperties.getOrElse("java.version", "") + " " +
            java.systemProperties.getOrElse("java.vendor", ""))),
      div(clear.both))
}

object SchedulerOverviewHtmlPage {
  object implicits {
    import scala.language.implicitConversions

    implicit def schedulerOverviewToHtmlPage(implicit webServiceContext: WebServiceContext) =
      ToHtmlPage[Snapshot[SchedulerOverview]] { (snapshot, pageUri) ⇒
        Future.successful(new SchedulerOverviewHtmlPage(snapshot, pageUri, webServiceContext.uris))
      }
  }
}
