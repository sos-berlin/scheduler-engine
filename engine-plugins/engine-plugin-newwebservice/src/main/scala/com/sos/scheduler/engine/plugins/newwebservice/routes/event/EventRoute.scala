package com.sos.scheduler.engine.plugins.newwebservice.routes.event

import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.common.sprayutils.SprayUtils.asFromStringOptionDeserializer
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, Event, EventSeq, Snapshot}
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat
import com.sos.scheduler.engine.kernel.DirectSchedulerClient
import com.sos.scheduler.engine.kernel.event.OrderStatisticsChangedSource
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.completeTryHtml
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.routes.event.EventRoutes._
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.KeyedEventsHtmlPage.implicits.keyedEventsToHtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.jsonToYamlHtmlPage
import scala.collection.immutable.Seq
import scala.concurrent.ExecutionContext
import scala.reflect.ClassTag
import spray.json.DefaultJsonProtocol._
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
trait EventRoute extends HasCloser with OrderEventRoute {

  protected def orderStatisticsChangedSource: OrderStatisticsChangedSource
  protected implicit def client: DirectSchedulerClient
  protected implicit def webServiceContext: WebServiceContext
  protected implicit def executionContext: ExecutionContext

  def eventRoute: Route =
    parameter("return".?) {
      case Some("OrderStatisticsChanged") ⇒
        orderPathEventRoute
      case _ ⇒
        otherEventRoute
    }

  private def otherEventRoute: Route =
    withEventParameters { case EventParameters(returnType, afterEventId, timeout, limit) ⇒
      pathSingleSlash {
        val classTag = ClassTag[Event](SchedulerAnyKeyedEventJsonFormat.typeToClass(returnType))
        if (limit >= 0)
          completeTryHtml[EventSeq[Seq, AnyKeyedEvent]] {
            client.events[Event](after = afterEventId, timeout, limit = limit)(classTag)
          }
        else
          completeTryHtml[Seq[Snapshot[AnyKeyedEvent]]] {
            client.eventsReverse[Event](after = afterEventId, limit = -limit)(classTag)
          }
      }
    }
}
