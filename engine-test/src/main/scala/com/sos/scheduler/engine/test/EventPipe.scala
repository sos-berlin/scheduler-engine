package com.sos.scheduler.engine.test

import com.google.common.collect.HashMultiset
import com.sos.scheduler.engine.base.utils.ScalaUtils.implicitClass
import com.sos.scheduler.engine.common.scalautil.AutoClosing.closeOnError
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, Event, KeyedEvent}
import com.sos.scheduler.engine.data.scheduler.SchedulerTerminatedEvent
import com.sos.scheduler.engine.eventbus._
import com.sos.scheduler.engine.test.EventPipe._
import java.time.Duration
import java.time.Instant.now
import java.util.concurrent.{LinkedBlockingQueue, TimeUnit}
import scala.annotation.tailrec
import scala.collection.JavaConversions._
import scala.collection.{immutable, mutable}
import scala.reflect.ClassTag

final class EventPipe(eventBus: SchedulerEventBus, defaultTimeout: Duration)
extends EventHandlerAnnotated with HasCloser {

  private val queue = new LinkedBlockingQueue[AnyKeyedEvent]

  closeOnError(closer) {
    eventBus.on[Event] { case e ⇒ queue.add(e) }
  }

  def next[E <: Event: ClassTag](keys: Iterable[E#Key]): immutable.Seq[KeyedEvent[E]] = {
    val remainingKeys = HashMultiset.create(asJavaIterable(keys))
    val events = mutable.Map[E#Key, KeyedEvent[E]]()
    while (!remainingKeys.isEmpty) {
      val e = nextAny[E]
      if (remainingKeys.remove(e.key)) {
        events += e.key → e
      }
    }
    keys.map(events).toVector
  }

  def next[E <: Event: ClassTag](key: E#Key, timeout: Duration = defaultTimeout): E =
    nextEvent[E](_.key == key, timeout).event

  def nextWhen[E <: Event : ClassTag](predicate: KeyedEvent[E] ⇒ Boolean, timeout: Duration = defaultTimeout): KeyedEvent[E] =
    nextEvent[E](predicate, timeout)

  def nextAny[E <: Event: ClassTag]: KeyedEvent[E] =
    nextEvent[E]()

  def queued[E <: Event: ClassTag]: immutable.Seq[KeyedEvent[E]] = {
    val result = mutable.ListBuffer[KeyedEvent[E]]()
    try while (true) result += nextEvent[E](timeout = 0.s)
    catch { case _: TimeoutException ⇒ }
    result.toList
  }

  private def nextEvent[E <: Event: ClassTag](predicate: KeyedEvent[E] ⇒ Boolean = EveryEvent, timeout: Duration = defaultTimeout): KeyedEvent[E] = {
    val expectedEventClass = implicitClass[E]
    val until = now() + timeout
    def expectedName = expectedEventClass.getSimpleName

    @tailrec def waitForEvent(): KeyedEvent[E] =
      tryPoll(until - now()) match {
        case None ⇒
          throw new TimeoutException(s"Expected event '$expectedName' has not arrived within ${timeout.pretty}")
        case Some(KeyedEvent(_, e: SchedulerTerminatedEvent)) ⇒
          sys.error(s"Expected event '$expectedName' has not arrived before ${classOf[SchedulerTerminatedEvent].getName} has arrived")
        case Some(e: AnyKeyedEvent) if (expectedEventClass isAssignableFrom e.event.getClass) && evalPredicateIfDefined[E](predicate, e.asInstanceOf[KeyedEvent[E]]) ⇒
          e.asInstanceOf[KeyedEvent[E]]
        case _ ⇒
          waitForEvent()
      }

    waitForEvent()
  }

  def poll(t: Duration): AnyKeyedEvent =
    tryPoll(t) getOrElse sys.error(s"Event has not arrived within ${t.pretty}")

  def tryPoll(t: Duration): Option[AnyKeyedEvent] =
    Option(queue.poll(t.toMillis, TimeUnit.MILLISECONDS))
}

object EventPipe {
  private val EveryEvent = (_: AnyKeyedEvent) ⇒ true

  class TimeoutException(override val getMessage: String) extends RuntimeException

  private def evalPredicateIfDefined[E <: Event](predicate: KeyedEvent[E] ⇒ Boolean, event: KeyedEvent[E]): Boolean =
    predicate match {
      case pf: PartialFunction[KeyedEvent[E], Boolean] ⇒ PartialFunction.cond(event)(pf)
      case _ ⇒ predicate(event)
    }
}
