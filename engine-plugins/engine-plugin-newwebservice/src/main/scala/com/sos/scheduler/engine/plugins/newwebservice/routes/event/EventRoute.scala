package com.sos.scheduler.engine.plugins.newwebservice.routes.event

import com.sos.scheduler.engine.common.event.collector.EventDirectives._
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.event.Event
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives._
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.KeyedEventsHtmlPage.implicits.keyedEventsToHtmlPage
import scala.concurrent.ExecutionContext
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
trait EventRoute extends HasCloser {

  protected implicit def client: DirectSchedulerClient
  protected implicit def webServiceContext: WebServiceContext
  protected implicit def executionContext: ExecutionContext

  def eventRoute: Route =
    pathEnd {
      eventRequest(classOf[Event], defaultReturnType = Some("Event")).apply { request ⇒
        completeTryHtml {
          client.events(request)
        }
      }
    }
}
