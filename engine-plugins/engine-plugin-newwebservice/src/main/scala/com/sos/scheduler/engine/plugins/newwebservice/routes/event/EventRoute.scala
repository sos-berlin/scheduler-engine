package com.sos.scheduler.engine.plugins.newwebservice.routes.event

import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.event._
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives._
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.event.EventRoutes._
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
      eventRequest(classOf[Event], defaultReturnType = Some("Event")) { request ⇒
        completeTryHtml {
          request match {
            case request: EventRequest[_] ⇒
              client.events(request)
            case request: ReverseEventRequest[_] ⇒
              for (responseSnapshot ← client.eventsReverse(request)) yield
                for (events ← responseSnapshot) yield
                  EventSeq.NonEmpty(events)
          }
        }
      }
    }
}
