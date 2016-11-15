package com.sos.scheduler.engine.plugins.newwebservice.routes.event

import com.sos.scheduler.engine.common.sprayutils.SprayUtils._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.event._
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat.anyEventJsonFormat
import java.time.Duration
import scala.collection.immutable.Seq
import shapeless.{::, HNil}
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

  def withEventParameters(defaultReturnType: Option[String] = None): Directive1[EventParameters] =
    new Directive1[EventParameters] {
      def happly(inner: EventParameters :: HNil ⇒ Route) =
        parameter("return".?) {
          _ orElse defaultReturnType match {
            case Some(returnType) ⇒
              passSome(anyEventJsonFormat.typeNameToClass.get(returnType)) { eventClass ⇒
                parameter("limit" ? Int.MaxValue) {
                  case 0 ⇒
                    reject(ValidationRejection(s"Invalid limit=0"))
                  case limit if limit > 0 ⇒
                    parameter("after".as[EventId]) { after ⇒
                      parameter("timeout" ? 0.s) { timeout ⇒
                        inner(EventParameters(eventClass, after, timeout, limit) :: HNil)
                      }
                    }
                  case limit if limit < 0 ⇒
                    parameter("after" ? EventId.BeforeFirst) { after ⇒
                      inner(EventParameters(eventClass, after, 0.s, limit) :: HNil)
                    }
                }
              }
            case None ⇒
              reject
            }
        }
    }

  final case class EventParameters(
    eventClass: Class[_ <: Event],
    after: EventId,
    timeout: Duration,
    limit: Int)
}
