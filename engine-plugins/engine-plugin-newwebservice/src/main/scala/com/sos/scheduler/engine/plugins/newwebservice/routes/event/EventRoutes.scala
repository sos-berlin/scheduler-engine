package com.sos.scheduler.engine.plugins.newwebservice.routes.event

import com.sos.scheduler.engine.base.utils.ScalaUtils.implicitClass
import com.sos.scheduler.engine.client.api.SchedulerOverviewClient
import com.sos.scheduler.engine.common.event.collector.EventDirectives._
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.common.utils.IntelliJUtils.intelliJuseImports
import com.sos.scheduler.engine.data.event._
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat.anyEventJsonFormat
import com.sos.scheduler.engine.data.events.schedulerKeyedEventJsonFormat
import com.sos.scheduler.engine.kernel.event.DirectEventClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.completeTryHtml
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.SingleKeyEventHtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.jsonToYamlHtmlPage
import scala.collection.immutable.Seq
import scala.concurrent.ExecutionContext
import scala.reflect.ClassTag
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
private[routes] object EventRoutes {
  intelliJuseImports(anyEventJsonFormat)

  // Nests a simple Snapshot[NoKeyEvent] into the expected nested type for the event web service.
  def nestIntoSeqSnapshot[E <: NoKeyEvent](snapshot: Snapshot[E]): Snapshot[EventSeq[Seq, AnyKeyedEvent]] = {
    val eventId = snapshot.eventId
    val anyKeyedEvent = KeyedEvent(snapshot.value).asInstanceOf[AnyKeyedEvent]
    Snapshot(eventId, EventSeq.NonEmpty(List(Snapshot(eventId, anyKeyedEvent))))
  }

  def singleKeyEvents[E <: Event: ClassTag](key: E#Key, defaultReturnType: Option[String] = None)
    (implicit client: DirectEventClient with SchedulerOverviewClient, webServiceContext: WebServiceContext, ec: ExecutionContext)
  : Route =
    eventRequest(implicitClass[E], defaultReturnType = defaultReturnType) { request ⇒
      completeTryHtml {
        implicit val toHtmlPage = SingleKeyEventHtmlPage.singleKeyEventToHtmlPage[AnyEvent](key)
        request match {
          case request: EventRequest[_] ⇒
            client.eventsForKey[AnyEvent](request.asInstanceOf[EventRequest[AnyEvent]], key)
          case request: ReverseEventRequest[_] ⇒
            for (responseSnapshot ← client.eventsReverseForKey[AnyEvent](request.asInstanceOf[ReverseEventRequest[AnyEvent]], key)) yield
              for (events ← responseSnapshot) yield
                EventSeq.NonEmpty(events)
        }
      }
    }

  def events[E <: Event: ClassTag](predicate: KeyedEvent[E] ⇒ Boolean, defaultReturnType: Option[String] = None)
    (implicit client: DirectEventClient with SchedulerOverviewClient, webServiceContext: WebServiceContext, ec: ExecutionContext)
  : Route =
    eventRequest(implicitClass[E], defaultReturnType = defaultReturnType) {
      case request: SomeEventRequest[_] ⇒
        val castRequest = request.asInstanceOf[SomeEventRequest[E]]
        completeTryHtml {
          client.eventsByPredicate[E](castRequest, predicate = predicate)
        }
      case _ ⇒
        reject
    }
}
