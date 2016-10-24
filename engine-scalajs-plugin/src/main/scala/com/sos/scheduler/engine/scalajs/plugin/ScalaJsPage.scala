package com.sos.scheduler.engine.scalajs.plugin

import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.client.web.SchedulerUris
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlPage.seqFrag
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.HtmlIncluder
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.HtmlIncluder.{toCssLinkHtml, toScriptHtml}
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.WebjarsRoute.NeededWebjars
import com.sos.scheduler.engine.scalajs.plugin.ScalaJsPage._
import scalatags.Text.all._
import scalatags.Text.{TypedTag, tags2}
import spray.http.Uri

/**
  * @author Joacim Zschimmer
  */
trait ScalaJsPage {
  def wholePage: TypedTag[String]
  protected val uris: SchedulerUris

  private lazy val includer = new HtmlIncluder(uris)

  protected def htmlPage(innerBody: Frag*): TypedTag[String] =
    html(lang := "en")(
      head(htmlHeadFrag),
      body(pageBody(innerBody :_*)))

  protected def htmlHeadFrag: Frag =
    seqFrag(
      meta(httpEquiv := "X-UA-Compatible", content := "IE=edge"),
      meta(name := "viewport", content := "width=device-width, initial-scale=1"),
      tags2.title("JobScheduler Scala.js"),
      css,
      javascript,
      link(rel := "icon", "sizes".attr := "64x64", `type` := "image/vnd.microsoft.icon",
        href := (uris / "api/frontend/common/images/jobscheduler.ico").toString))

  private def css: Frag =
    (NeededWebjars map includer.cssHtml) ++
      (cssLinks map toCssLinkHtml)

  private def javascript: Frag =
    (NeededWebjars map includer.javascriptHtml) ++
      (scriptLinks map toScriptHtml)

  protected def cssLinks: Vector[Uri] = Vector(uris / "api/frontend/common/common.css")
  protected def scriptLinks: Vector[Uri] = Vector(uris / "api/frontend/common/common.js")

  protected def pageBody(innerBody: Frag*): Frag =
    seqFrag(
      navbar,
      div(cls := "container", width := "100%")(
        innerBody))

  private def navbar: Frag =
    nav(cls := "navbar navbar-default navbar-static-top")(
      div(cls := "container-fluid")(
        div(cls := "navbar-header")(
          uncollapseButton,
          brand),
        div(cls := "collapse navbar-collapse")(
          menu,
          div(cls := "navbar-text navbar-right")(
            "(time)"))))

  private def brand: Frag =
    a(cls := "navbar-brand", position.relative, top := (-9).px, whiteSpace.nowrap, href := uris.overview)(
      table(
        tbody(
          tr(
            td(rowspan := 2, paddingRight := 1.ex)(
              img("width".attr := 40, "height".attr := 40, alt := "Rabbit",
                src := uris.uriString("api/frontend/common/images/job_scheduler_rabbit_circle_60x60.gif"))),
            td(
              span(" JobScheduler"))),
          tr(
            td(fontSize := 13.px)(
              "(version)")))))

  private def menu: Frag =
    ul(cls := "nav navbar-nav ")(
      navBarTab("(menu)", "#"))

  private def uncollapseButton: Frag =
    button(`type` := "button", cls := "navbar-toggle", data("toggle") := "collapse", data("target") := ".navbar-collapse")(
      span(cls := "icon-bar"),
      span(cls := "icon-bar"),
      span(cls := "icon-bar"))

  private def navBarTab(label: String, relativeUri: String) = {
    val uri = uris / relativeUri
    val isActive = false
    li(role := "presentation", isActive option { cls := "active" })(
      a(href := uri.toString)(label))
  }
}

object ScalaJsPage {
  private lazy val nav = "nav".tag[String]
}
