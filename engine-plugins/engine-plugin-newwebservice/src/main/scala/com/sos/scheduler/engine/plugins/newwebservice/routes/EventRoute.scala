package com.sos.scheduler.engine.plugins.newwebservice.routes

import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.event.EventId
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.completeTryHtml
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.EventsHtmlPage.implicits.eventsToHtmlPage
import scala.concurrent.ExecutionContext
import com.sos.scheduler.engine.data.events.EventJsonFormat
import spray.json.DefaultJsonProtocol._
import spray.routing.Directives._
import spray.routing.Route

/**
  * @author Joacim Zschimmer
  */
trait EventRoute extends HasCloser {

  protected implicit def client: DirectSchedulerClient
  protected implicit def webServiceContext: WebServiceContext
  protected implicit def executionContext: ExecutionContext

  def eventRoute: Route =
    pathSingleSlash {
      parameter("after".?) { afterEventIdOption ⇒
        val afterEventId = afterEventIdOption map EventId.apply getOrElse EventId.BeforeFirst
        completeTryHtml(client.events(afterEventId))
      }
    }
}
