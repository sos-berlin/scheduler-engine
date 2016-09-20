package com.sos.scheduler.engine.kernel.event

import com.sos.scheduler.engine.data.event.{KeyedTypedEventJsonFormat, _}
import com.sos.scheduler.engine.data.events.SchedulerAnyKeyedEventJsonFormat.eventTypedJsonFormat
import com.sos.scheduler.engine.data.events.schedulerKeyedEventJsonFormat
import com.sos.scheduler.engine.data.log.Logged
import com.sos.scheduler.engine.kernel.event.collector.EventCollector
import scala.collection.immutable.Seq
import scala.concurrent.{ExecutionContext, Future}
import scala.reflect.ClassTag

/**
  * @author Joacim Zschimmer
  */
trait DirectEventClient {
  protected def eventCollector: EventCollector
  protected implicit def executionContext: ExecutionContext

  def events[E <: Event: ClassTag](after: EventId, limit: Int = Int.MaxValue, reverse: Boolean = false): Future[Snapshot[Seq[Snapshot[KeyedEvent[E]]]]] = {
    val eventJsonFormat = implicitly[KeyedTypedEventJsonFormat[Event]]
    for (snapshotIterator ← eventCollector.when[E](after, reverse = reverse);
         wholeSnapshot ← {
           val wholeEventId = eventCollector.newEventId() // This EventId is only to give the response a timestamp. To continue the event stream, use the last event's EventId.
           var lastEventId = after
           val serializables = snapshotIterator filter { o ⇒
             lastEventId = o.eventId
             eventIsSelected(o.value) && (eventJsonFormat canSerialize o.value)
           } take limit
           if (serializables.isEmpty)
             events[E](after = lastEventId, limit = limit, reverse = reverse)
           else
             Future.successful(Snapshot(wholeEventId, serializables.toVector))
         })
      yield wholeSnapshot
  }

  private def eventIsSelected(event: AnyKeyedEvent): Boolean =
    event match {
      //case KeyedEvent(_, e: InfoOrHigherLogged) ⇒ true
      case KeyedEvent(_, e: Logged) ⇒ false
      case _ ⇒ true
    }

  def eventsForKey[E <: Event: ClassTag](key: E#Key, after: EventId, limit: Int = Int.MaxValue, reverse: Boolean = false): Future[Snapshot[Seq[Snapshot[E]]]] =
    for (Snapshot(eventId, events) ← eventCollector.whenForKey(key, after, reverse = reverse);
         wholeSnapshot ← {
           var lastEventId = EventId.BeforeFirst
           val serializables = events filter { o ⇒
             lastEventId = o.eventId
             eventTypedJsonFormat canSerialize o.value
           } take limit
           if (serializables.isEmpty)
             eventsForKey[E](key, after = lastEventId, limit = limit, reverse = reverse)
           else
             Future.successful(Snapshot(eventId, serializables.toVector))
         })
      yield wholeSnapshot
}
