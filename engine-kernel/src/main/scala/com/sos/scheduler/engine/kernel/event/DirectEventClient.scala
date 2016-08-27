package com.sos.scheduler.engine.kernel.event

import com.sos.scheduler.engine.data.event.KeyedEvent.KeyedTypedEventJsonFormat
import com.sos.scheduler.engine.data.event._
import com.sos.scheduler.engine.data.events.SchedulerKeyedEventJsonFormat
import com.sos.scheduler.engine.data.events.SchedulerKeyedEventJsonFormat.eventTypedJsonFormat
import com.sos.scheduler.engine.data.log.LogEvent
import scala.collection.immutable.Seq
import scala.concurrent.{ExecutionContext, Future}
import scala.reflect.ClassTag

/**
  * @author Joacim Zschimmer
  */
trait DirectEventClient {
  protected def eventCollector: EventCollector
  protected implicit def executionContext: ExecutionContext

  def events(after: EventId, limit: Int = Int.MaxValue, reverse: Boolean = false): Future[Snapshot[Seq[Snapshot[AnyKeyedEvent]]]] = {
    val eventJsonFormat = implicitly[KeyedTypedEventJsonFormat[Event]]
    for (iterator ← eventCollector.whenEvents(after, reverse = reverse)) yield {
      val eventId = eventCollector.newEventId()  // This EventId is only to give the response a timestamp. To continue the event stream, use the last event's EventId.
      val serializables = iterator filter { o ⇒ eventIsSelected(o.value) && (eventJsonFormat canSerialize o.value) } take limit
      //if (serializables.isEmpty)
        // TODO Restart in case no Event can be serialized: case Vector() ⇒ this.events(after)
      //else
        Snapshot(serializables.toVector)(eventId)
    }
  }

  private def eventIsSelected(event: AnyKeyedEvent): Boolean =
    event match {
      //case KeyedEvent(_, e: InfoOrHigherLogged) ⇒ true
      case KeyedEvent(_, e: LogEvent) ⇒ false
      case _ ⇒ true
    }

  def eventsForKey[E <: Event: ClassTag](key: E#Key, after: EventId, limit: Int = Int.MaxValue, reverse: Boolean = false): Future[Snapshot[Seq[Snapshot[E]]]] = {
    for (iterator ← eventCollector.whenEventsForKey(key, after, reverse = reverse)) yield {
      val eventId = eventCollector.newEventId()  // This EventId is only to give the response a timestamp. To continue the event stream, use the last event's EventId.
      val serializables = iterator filter { o ⇒ eventTypedJsonFormat canSerialize o.value } take limit
      //if (serializables.isEmpty)
        // TODO Restart in case no Event can be serialized: case Vector() ⇒ this.events(after)
      //else
        Snapshot(serializables.toVector)(eventId)
    }
  }
}
