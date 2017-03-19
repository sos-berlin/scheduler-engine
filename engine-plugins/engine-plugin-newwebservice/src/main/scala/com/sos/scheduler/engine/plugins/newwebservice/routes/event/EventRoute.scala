package com.sos.scheduler.engine.plugins.newwebservice.routes.event

import com.sos.jobscheduler.common.event.collector.EventDirectives._
import com.sos.jobscheduler.common.scalautil.HasCloser
import com.sos.jobscheduler.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.jobscheduler.common.sprayutils.html.HtmlDirectives
import com.sos.jobscheduler.data.event.Event
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerWebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.KeyedEventsHtmlPage.implicits.keyedEventsToHtmlPage
import scala.concurrent.ExecutionContext
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
trait EventRoute extends HasCloser with HtmlDirectives[SchedulerWebServiceContext] {

  protected implicit def client: DirectSchedulerClient
  protected implicit def webServiceContext: SchedulerWebServiceContext
  protected implicit def executionContext: ExecutionContext

  def eventRoute: Route =
    pathEnd {
      eventRequest[Event](defaultReturnType = Some("Event")).apply { request ⇒
        completeTryHtml {
          client.events(request)
        }
      }
    }
}
