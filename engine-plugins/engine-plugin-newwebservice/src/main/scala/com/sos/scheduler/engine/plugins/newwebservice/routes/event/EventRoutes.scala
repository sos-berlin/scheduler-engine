package com.sos.scheduler.engine.plugins.newwebservice.routes.event

import com.sos.scheduler.engine.common.sprayutils.SprayUtils._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.event._
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat.anyEventJsonFormat
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

  def eventRequest[E <: Event](eventSuperclass: Class[_ <: E], defaultReturnType: Option[String] = None): Directive1[SomeEventRequest[_ <: E]] =
    new Directive1[SomeEventRequest[_ <: E]] {
      def happly(inner: SomeEventRequest[_ <: E] :: HNil ⇒ Route) =
        parameter("return".?) {
          _ orElse defaultReturnType match {
            case Some(returnType) ⇒
              passSome(anyEventJsonFormat.typeNameToClass.get(returnType)) {
                case eventClass if eventSuperclass isAssignableFrom eventClass ⇒
                  val eClass = eventClass.asInstanceOf[Class[_ <: E]]
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
