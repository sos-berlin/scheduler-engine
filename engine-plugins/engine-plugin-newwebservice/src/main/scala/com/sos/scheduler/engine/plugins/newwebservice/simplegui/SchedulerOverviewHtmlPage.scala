package com.sos.scheduler.engine.plugins.newwebservice.simplegui

import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.common.utils.JavaResource
import com.sos.jobscheduler.data.event.Stamped
import com.sos.jobscheduler.data.system.JavaInformation
import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.scheduler.engine.data.queries.OrderQuery
import com.sos.scheduler.engine.data.scheduler.SchedulerOverview
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.ToHtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.HtmlIncluder.toVersionedUriPath
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.SchedulerHtmlPage.instantWithDurationToHtml
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.SchedulerOverviewHtmlPage._
import java.util.Locale.US
import scala.concurrent.Future
import scala.math.round
import scala.util.control.NonFatal
import scalatags.Text.all._
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
final class SchedulerOverviewHtmlPage private(
  protected val snapshot: Stamped[SchedulerOverview],
  protected val pageUri: Uri,
  protected val uris: SchedulerUris)
extends SchedulerHtmlPage {

  protected val schedulerOverview = snapshot.value
  import schedulerOverview.{java, startedAt, state, system}

  override protected def cssPaths = super.cssPaths ++ CssPaths
  override protected def scriptPaths = super.scriptPaths ++ ScriptPaths

  def wholePage =
    htmlPage(
      div(float.left, marginRight := 2.em)(
        schedulerInfoHtml),
      div(float.right)(
        systemInformationHtml),
      div(float.left)(
        orderStatistics),
      commandInput)

  private def systemInformationHtml =
    div(cls := "ContentBox SystemInformation")(
      table(cls := "SystemInformation")(
        tbody(
          tr(
            td(systemPropertiesHtml),
            td(cpuAndRamBarsHtml)),
          tr(
            td(javaPropertiesHtml),
            td(javaBarsHtml(java.memory))))))

  private def systemPropertiesHtml: Frag =
    div(cls := "SystemProperties")(
      p("Host ", system.hostname),
      for (o ← system.distribution) yield
        div(o),
      div(
        java.systemProperties.getOrElse("os.name", "") + " " +
          java.systemProperties.getOrElse("os.version", "")),
      for (o ← system.cpuModel) yield
        div(o))

  private def cpuAndRamBarsHtml: Option[Frag] =
    try
      for (os ← system.mxBeans.get("operatingSystem") map { _.asInstanceOf[Map[String, Any]] };
           total ← os.get("totalPhysicalMemorySize") map { _.asInstanceOf[Number].longValue } if total > 1000*1000;  // Avoid division by zero
           free ← os.get("freePhysicalMemorySize") map { _.asInstanceOf[Number].longValue };
           ratio = free.toDouble / total) yield
        div(cls := "SystemBar")(
          for (systemCpuLoad ← os.get("systemCpuLoad") map { _.asInstanceOf[Number] };
               processorCount ← os.get("availableProcessors") map { _.asInstanceOf[Number] }) yield
            div(
              div(
                small(s"CPU $processorCount×"),
                for (o ← java.systemProperties.get("os.arch")) yield small(s" $o")),
              progress(
                progressBar(systemCpuLoad.doubleValue)(
                  round(systemCpuLoad.doubleValue * 100).toInt + "% CPU"))),
          List(
            small(toMiB(total) + " RAM ∙ " + toMiB(free) + " free"),
            progress(
              progressBar(1 - ratio)(
                toMiB(total - free)),
              progressBar(ratio)(backgroundColor := "inherit")(
                span(color.black)(toMiB(free))))))
    catch {
      case NonFatal(t) ⇒
        logger.debug(s"cpuAndRamBarsHtml: $t")
        None
  }

  private def javaPropertiesHtml =
    div(
      div("Java " + java.systemProperties.getOrElse("java.version", "")),
      for (o ← java.systemProperties.get("java.vendor")) yield div(s"($o)"))

  private def javaBarsHtml(memory: JavaInformation.Memory): Frag = {
    import memory.{free, maximum, reserve, used}
    div(cls := "SystemBar")(
      small(toMiB(java.memory.total) + " Heap ∙ " + toMiB(java.memory.free) + " free"),
      progress(
        progressBar(used / maximum.toDouble)(
          toMiB(used)),
        progressBar(free / maximum.toDouble)(cls := "progress-bar progress-bar-success")(
          toMiB(free)),
        progressBar(reserve / maximum.toDouble)(backgroundColor := "inherit")(
          span(color.black)(toMiB(reserve)))))
  }

  private def schedulerInfoHtml: Frag =
    div(cls := "SchedulerOverview")(
      div(
        "Started at ",
        instantWithDurationToHtml(startedAt),
        " · ", b(state.toString)))

  private def orderStatistics: Frag =
    new JocOrderStatisticsWidget(uris, OrderQuery.All).html

  private def commandInput: Frag =
    form(action := "api/command", method := "get", clear.both)(
      span(cls := "input-group input-group-sm")(
        span(cls := "input-group-addon")(
          "XML command: "),
        input(`type` := "text", /*autofocus (Enter triggers update),*/ name := "command", value := "s", placeholder := "For example: show_state", cls := "form-control"),
        " ",
        span(cls := "input-group-btn")(
          button(`type` := "submit", cls := "btn btn-primary")(
            "Execute"))))
}

object SchedulerOverviewHtmlPage {
  private val logger = Logger(getClass)
  private val ThinSpace = '\u2009'
  private val CssPaths = Vector(
    toVersionedUriPath(JavaResource("com/sos/scheduler/engine/plugins/newwebservice/simplegui/frontend/common/OrderStatisticsWidget.css")),
    toVersionedUriPath(JavaResource("com/sos/scheduler/engine/plugins/newwebservice/simplegui/frontend/schedulerOverview/overview.css")))
  private val ScriptPaths = Vector(
    toVersionedUriPath(JavaResource("com/sos/scheduler/engine/plugins/newwebservice/simplegui/frontend/common/OrderStatisticsWidget.js")))

  object implicits {
    import scala.language.implicitConversions

    implicit def schedulerOverviewToHtmlPage(implicit webServiceContext: WebServiceContext) =
      ToHtmlPage[Stamped[SchedulerOverview]] { (snapshot, pageUri) ⇒
        Future.successful(new SchedulerOverviewHtmlPage(snapshot, pageUri, webServiceContext.uris))
      }
  }

  private def progress = div(cls := "progress", marginBottom := 5.px)

  private def progressBar(size: Double) = div(cls := "progress-bar", width := toPercent(size - 0.0001))  // Minor subtraction to be not greater than 100 %

  private def toPercent(a: Double) = "%3.2f".formatLocal(US, 100 * a) + "%"

  private def toMiB(n: Long) =
    if (n < 1024*1024*1024)
      toUnit(n, "MiB", 1024 * 1024)
    else
      toUnit(n, "GiB", 1024 * 1024 * 1024)

  private def toUnit(n: Long, unit: String, factor: Int) = {
    val nn = n / factor.toDouble
    val fractionDigits = if (nn <= 2) 1 else 0
    s"%.${fractionDigits}f".formatLocal(US, nn) + s"$ThinSpace$unit"
  }
}
