package com.sos.scheduler.engine.kernel.event.collector

import com.sos.scheduler.engine.base.utils.ScalaUtils.implicitClass
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.RichFutureFuture
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, Event, EventId, EventSeq, KeyedEvent, Snapshot}
import com.sos.scheduler.engine.eventbus.SchedulerEventBus
import com.sos.scheduler.engine.kernel.event.collector.EventCollector._
import com.typesafe.config.Config
import java.time.Duration
import java.time.Instant.now
import javax.inject.{Inject, Singleton}
import scala.concurrent.{ExecutionContext, Future}
import scala.reflect.ClassTag

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class EventCollector @Inject()(
  val eventIdGenerator: EventIdGenerator,
  eventBus: SchedulerEventBus,
  configuration: Configuration = Configuration.Default)
  (implicit ec: ExecutionContext)
extends HasCloser {

  private[collector] val keyedEventQueue = new KeyedEventQueue(sizeLimit = configuration.queueSize)
  //private val keyToEventQueue = new concurrent.TrieMap[Any, EventQueue]()

  private val sync = new Sync {
    def hasAfter(eventId: EventId) = keyedEventQueue.hasAfter(eventId)
  }

  eventBus.onHot[Event] { case keyedEvent ⇒
    //keyToEventQueue.getOrElseUpdate(keyedEvent.key, new EventQueue(EventQueueSizeLimitPerKey))
    //  .add(Snapshot(eventId, keyedEvent.event))
    keyedEventQueue.add(eventIdGenerator.newSnapshot(keyedEvent))
    sync.onNewEvent()
  }

  def when[E <: Event: ClassTag](
    after: EventId,
    timeout: Duration,
    predicate: KeyedEvent[E] ⇒ Boolean = (_: KeyedEvent[E]) ⇒ true,
    limit: Int = Int.MaxValue)
  : Future[EventSeq[Iterator, KeyedEvent[E]]]
  =
    whenAny[E](Set(implicitClass[E]), after, timeout, predicate, limit)

  def whenAny[E <: Event](
    eventClasses: Set[Class[_ <: E]],
    after: EventId,
    timeout: Duration,
    predicate: KeyedEvent[E] ⇒ Boolean = (_: KeyedEvent[E]) ⇒ true,
    limit: Int = Int.MaxValue)
  : Future[EventSeq[Iterator, KeyedEvent[E]]]
  =
    whenAnyKeyedEvents(
      after,
      timeout,
      collect = {
        case e if eventClasses.exists(_ isAssignableFrom e.event.getClass) && predicate(e.asInstanceOf[KeyedEvent[E]]) ⇒
          e.asInstanceOf[KeyedEvent[E]]
      },
      limit)

  def whenForKey[E <: Event: ClassTag](
    key: E#Key,
    after: EventId,
    timeout: Duration,
    predicate: E ⇒ Boolean = (_: E) ⇒ true,
    limit: Int = Int.MaxValue)
  : Future[EventSeq[Iterator, E]]
  =
    whenAnyKeyedEvents(
      after,
      timeout,
      collect = {
        case e if (implicitClass[E] isAssignableFrom e.event.getClass) && e.key == key && predicate(e.event.asInstanceOf[E]) ⇒
          e.event.asInstanceOf[E]
      },
      limit)

  private def whenAnyKeyedEvents[A](after: EventId, timeout: Duration, collect: PartialFunction[AnyKeyedEvent, A], limit: Int): Future[EventSeq[Iterator, A]] = {
    val until = now + timeout
    def loop(): Future[EventSeq[Iterator, A]] =
      (for (() ← sync.whenEventIsAvailable(after)) yield
        collectEventsSince(after, collect, limit) match {
          case o @ EventSeq.NonEmpty(collected) ⇒
            Future.successful(o)
          case EventSeq.Empty(lastEventId) if now < until ⇒
            loop()
          case EventSeq.Empty(lastEventId) ⇒
            Future.successful(EventSeq.Empty(lastEventId))
          case EventSeq.Torn ⇒
            Future.successful(EventSeq.Torn)
      }).flatten
    loop()
  }

  private def collectEventsSince[A](after: EventId, collect: PartialFunction[AnyKeyedEvent, A], limit: Int): EventSeq[Iterator, A] = {
    require(limit >= 0, "limit must be >= 0")
    val eventSnapshotOption = keyedEventQueue.after(after)
    var lastEventId = after
    eventSnapshotOption match {
      case Some(eventSnapshots) ⇒
        val eventIterator =
          eventSnapshots
            .map { o ⇒ lastEventId = o.eventId; o }
            .collect {
              case snapshot if collect.isDefinedAt(snapshot.value) ⇒
                Snapshot(snapshot.eventId, collect(snapshot.value))
            }
            .take(limit)
        if (eventIterator.nonEmpty)
          EventSeq.NonEmpty(eventIterator)
        else
          EventSeq.Empty(lastEventId)
      case None ⇒
        EventSeq.Torn
    }
  }

  def reverse[E <: Event: ClassTag](
    after: EventId = EventId.BeforeFirst,
    predicate: KeyedEvent[E] ⇒ Boolean = (_: KeyedEvent[E]) ⇒ true,
    limit: Int = Int.MaxValue)
  : Iterator[Snapshot[KeyedEvent[E]]]
  =
    keyedEventQueue.reverseEvents(after = after)
      .collect {
        case snapshot if implicitClass[E].isAssignableFrom(snapshot.value.event.getClass) && predicate(snapshot.value.asInstanceOf[KeyedEvent[E]]) ⇒
          snapshot.asInstanceOf[Snapshot[KeyedEvent[E]]]
      }
      .take(limit)

  def newSnapshot[A](a: A) = eventIdGenerator.newSnapshot(a)
}

object EventCollector {
  final case class Configuration(queueSize: Int)

  object Configuration {
    val Default = Configuration(queueSize = 10000)

    def fromSubConfig(config: Config) = Configuration(
      queueSize = config.getInt("queue-size")
    )
  }
}
