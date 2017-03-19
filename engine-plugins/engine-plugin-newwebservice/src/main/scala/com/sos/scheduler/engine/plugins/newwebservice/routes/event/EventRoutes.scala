package com.sos.scheduler.engine.plugins.newwebservice.routes.event

import com.sos.jobscheduler.common.event.collector.EventDirectives._
import com.sos.jobscheduler.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.jobscheduler.common.sprayutils.html.HtmlDirectives
import com.sos.jobscheduler.common.utils.IntelliJUtils.intelliJuseImports
import com.sos.jobscheduler.data.event._
import com.sos.scheduler.engine.client.api.SchedulerOverviewClient
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat.anyEventJsonFormat
import com.sos.scheduler.engine.data.events.schedulerKeyedEventJsonFormat
import com.sos.scheduler.engine.kernel.event.DirectEventClient
import com.sos.scheduler.engine.plugins.newwebservice.html.SchedulerWebServiceContext
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
private[routes] trait EventRoutes extends HtmlDirectives[SchedulerWebServiceContext]  {
  intelliJuseImports(anyEventJsonFormat)

  // Nests a simple Stamped[NoKeyEvent] into the expected nested type for the event web service.
  def nestIntoSeqStamped[E <: NoKeyEvent](stamped: Stamped[E]): Stamped[TearableEventSeq[Seq, AnyKeyedEvent]] = {
    val eventId = stamped.eventId
    val anyKeyedEvent = KeyedEvent(stamped.value).asInstanceOf[AnyKeyedEvent]
    Stamped(eventId, EventSeq.NonEmpty(List(Stamped(eventId, anyKeyedEvent))))
  }

  def singleKeyEvents[E <: Event: ClassTag](key: E#Key, defaultReturnType: Option[String] = None)
    (implicit client: DirectEventClient with SchedulerOverviewClient, webServiceContext: SchedulerWebServiceContext, ec: ExecutionContext)
  : Route =
    eventRequest[E](defaultReturnType = defaultReturnType).apply { request ⇒
      completeTryHtml {
        implicit val toHtmlPage = SingleKeyEventHtmlPage.singleKeyEventToHtmlPage[AnyEvent](key)
        request match {
          case request: EventRequest[_] ⇒
            client.eventsForKey[AnyEvent](request.asInstanceOf[EventRequest[AnyEvent]], key)
          case request: ReverseEventRequest[_] ⇒
            for (stamped ← client.eventsReverseForKey[AnyEvent](request.asInstanceOf[ReverseEventRequest[AnyEvent]], key)) yield
              for (events ← stamped) yield
                EventSeq.NonEmpty(events)
        }
      }
    }

  def events[E <: Event: ClassTag](predicate: KeyedEvent[E] ⇒ Boolean, defaultReturnType: Option[String] = None)
    (implicit client: DirectEventClient with SchedulerOverviewClient, webServiceContext: SchedulerWebServiceContext, ec: ExecutionContext)
  : Route =
    eventRequest[E](defaultReturnType = defaultReturnType).apply {
      case request: SomeEventRequest[_] ⇒
        val castRequest = request.asInstanceOf[SomeEventRequest[E]]
        completeTryHtml {
          client.eventsByPredicate[E](castRequest, predicate = predicate)
        }
      case _ ⇒
        reject
    }
}
