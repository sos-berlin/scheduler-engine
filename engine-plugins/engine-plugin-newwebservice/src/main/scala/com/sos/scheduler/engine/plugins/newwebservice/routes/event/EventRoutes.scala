package com.sos.scheduler.engine.plugins.newwebservice.routes.event

import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.event._
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

  object withEventParameters extends Directive1[EventParameters] {
    def happly(inner: EventParameters :: HNil ⇒ Route) =
      parameter("return" ? classOf[Event].getSimpleName) { returnType ⇒
        parameter("after" ? EventId.BeforeFirst) { after ⇒
          parameter("timeout" ? 0.s) { timeout ⇒
            parameter("limit" ? Int.MaxValue) { limit ⇒
              if (limit == 0)
                reject(ValidationRejection(s"Invalid limit=$limit"))
              else
                inner(EventParameters(returnType, after, timeout, limit) :: HNil)
            }
          }
        }
      }
    }

  final case class EventParameters(
    returnType: String,
    after: EventId,
    timeout: Duration,
    limit: Int)
}
