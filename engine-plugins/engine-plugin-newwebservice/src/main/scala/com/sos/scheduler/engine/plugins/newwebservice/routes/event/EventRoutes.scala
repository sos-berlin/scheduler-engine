package com.sos.scheduler.engine.plugins.newwebservice.routes.event

import com.sos.scheduler.engine.base.convert.As._
import com.sos.scheduler.engine.base.convert.ConvertiblePartialFunctions.ImplicitConvertablePF
import com.sos.scheduler.engine.data.event.{KeyedEvent, _}
import scala.collection.immutable.Seq
import shapeless.{::, HNil}
import spray.routing.Directives._
import spray.routing.{ValidationRejection, _}

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

  object withEventParameters extends Directive1[EventParameters] {
    def happly(inner: EventParameters :: HNil ⇒ Route) =
      parameterMap { parameters ⇒
        val returnType = parameters.as[String]("return", classOf[Event].getSimpleName)
        val afterEventId = parameters.as[Long]("after", EventId.BeforeFirst)
        val limit = parameters.as[Int]("limit", Int.MaxValue)
        if (limit == 0)
          reject(ValidationRejection(s"Invalid limit=$limit"))
        else
          inner(EventParameters(returnType, afterEventId, limit) :: HNil)
      }
    }

  final case class EventParameters(returnType: String, after: EventId, limit: Int)
}
