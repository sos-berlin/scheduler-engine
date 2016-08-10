package com.sos.scheduler.engine.plugins.newwebservice.routes.event

import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.data.event.{EventId, IdAndEvent}
import com.sos.scheduler.engine.data.events.EventJsonFormat
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.completeTryHtml
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import scala.concurrent.ExecutionContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.toYamlHtmlPage
import spray.json.DefaultJsonProtocol._
import spray.routing.Directives._
import spray.routing.Route

/**
  * @author Joacim Zschimmer
  */
trait EventRoute extends HasCloser {

  protected def eventCollector: EventCollector
  protected implicit def client: DirectSchedulerClient
  protected implicit def webServiceContext: WebServiceContext
  protected implicit def executionContext: ExecutionContext

  def eventRoute: Route =
    pathSingleSlash {
      parameter("after".?) { afterEventIdOption ⇒
        val lastEventId = afterEventIdOption map EventId.apply getOrElse EventId.BeforeFirst
        completeTryHtml(eventCollector.iteratorFuture(lastEventId) map { _.filter(IdAndEvent.canSerialize).toVector })
      }
    }
}
