package com.sos.scheduler.engine.plugins.newwebservice.routes.event

import com.sos.scheduler.engine.base.utils.ScalaUtils
import com.sos.scheduler.engine.base.utils.ScalaUtils.implicitClass
import com.sos.scheduler.engine.common.sprayutils.SprayUtils.asFromStringOptionDeserializer
import com.sos.scheduler.engine.client.api.SchedulerOverviewClient
import com.sos.scheduler.engine.common.sprayutils.SprayUtils._
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.event._
import com.sos.scheduler.engine.data.events.schedulerKeyedEventJsonFormat
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat._
import com.sos.scheduler.engine.kernel.event.DirectEventClient
import com.sos.scheduler.engine.plugins.newwebservice.html.HtmlDirectives.completeTryHtml
import com.sos.scheduler.engine.plugins.newwebservice.html.WebServiceContext
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.SingleKeyEventHtmlPage
import com.sos.scheduler.engine.plugins.newwebservice.simplegui.YamlHtmlPage.implicits.jsonToYamlHtmlPage
import scala.collection.immutable.Seq
import scala.concurrent.ExecutionContext
import scala.reflect.ClassTag
import shapeless.{::, HNil}
import spray.json.DefaultJsonProtocol._
import spray.routing.Directives._
import spray.routing._

/**
  * @author Joacim Zschimmer
  */
private[routes] object EventRoutes {

  // Nests a simple Snapshot[NoKeyEvent] into the expected nested type for the event web service.
  def nestIntoSeqSnapshot[E <: NoKeyEvent](snapshot: Snapshot[E]): Snapshot[EventSeq[Seq, AnyKeyedEvent]] = {
    val eventId = snapshot.eventId
    val anyKeyedEvent = KeyedEvent(snapshot.value).asInstanceOf[AnyKeyedEvent]
    Snapshot(eventId, EventSeq.NonEmpty(List(Snapshot(eventId, anyKeyedEvent))))
  }

  def singleKeyEvents[E <: Event: ClassTag](key: E#Key, defaultReturnType: Option[String] = None)
    (implicit client: DirectEventClient with SchedulerOverviewClient, webServiceContext: WebServiceContext, ec: ExecutionContext)
  : Route =
    eventRequest(implicitClass[E]) { request ⇒
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
    eventRequest(implicitClass[E]) {
      case request: EventRequest[_] ⇒
        val castRequest = request.asInstanceOf[EventRequest[E]]
        completeTryHtml {
          client.eventsByPredicate[E](castRequest, predicate = predicate)
        }
      case _ ⇒
        reject
    }

  def eventRequest[E <: Event, EE <: E](eventSuperclass: Class[EE], defaultReturnType: Option[String] = None): Directive1[SomeEventRequest[EE]] =
    new Directive1[SomeEventRequest[EE]] {
      def happly(inner: SomeEventRequest[EE] :: HNil ⇒ Route) =
        parameter("return".?) {
          _ orElse defaultReturnType match {
            case Some(returnType) ⇒
              passSome(anyEventJsonFormat.typeNameToClass.get(returnType)) {
                case eventClass if eventSuperclass isAssignableFrom eventClass ⇒
                  val eClass = eventClass.asInstanceOf[Class[EE]]
                  parameter("limit" ? Int.MaxValue) {
                    case 0 ⇒
                      reject(ValidationRejection(s"Invalid limit=0"))
                    case limit if limit > 0 ⇒
                      parameter("after".as[EventId]) { after ⇒
                        parameter("timeout" ? 0.s) { timeout ⇒
                          inner(EventRequest(eClass, after = after, timeout, limit = limit) :: HNil)
                        }
                      }
                    case limit if limit < 0 ⇒
                      parameter("after" ? EventId.BeforeFirst) { after ⇒
                        inner(ReverseEventRequest(eClass, after = after, limit = -limit) :: HNil)
                      }
                  }
                case _ ⇒
                  reject
              }
            case None ⇒
              reject
          }
        }
    }
}
