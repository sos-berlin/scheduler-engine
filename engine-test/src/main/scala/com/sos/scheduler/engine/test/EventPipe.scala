package com.sos.scheduler.engine.test

import com.google.common.collect.HashMultiset
import com.sos.scheduler.engine.base.utils.ScalaUtils.implicitClass
import com.sos.scheduler.engine.common.scalautil.AutoClosing.closeOnError
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, KeyedEvent, Event}
import com.sos.scheduler.engine.eventbus._
import com.sos.scheduler.engine.main.event.TerminatedEvent
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

  def nextKeyedEvents[E <: Event: ClassTag](keys: Iterable[E#Key]): immutable.Seq[KeyedEvent[E]] = {
    val remainingKeys = HashMultiset.create(asJavaIterable(keys))
    val events = mutable.Buffer[KeyedEvent[E]]()
    while (!remainingKeys.isEmpty) {
      val e = nextAny[E]
      if (remainingKeys.remove(e.key)) {
        events += e
      }
    }
    events.toVector
  }

  def nextAny[E <: Event: ClassTag]: KeyedEvent[E] =
    _next[KeyedEvent[E]](e ⇒ implicitClass[E] isAssignableFrom e.event.getClass)

  def nextKeyed[E <: Event: ClassTag](key: E#Key, timeout: Duration = defaultTimeout): E =
    _next[KeyedEvent[E]](isKeyedEvent[E](key), timeout).event

  def nextWithCondition[E <: Event : ClassTag](condition: KeyedEvent[E] ⇒ Boolean): KeyedEvent[E] =
    nextWithTimeoutAndCondition[E](condition)

  def nextWithTimeoutAndCondition[E <: Event : ClassTag](condition: KeyedEvent[E] ⇒ Boolean, timeout: Duration = defaultTimeout): KeyedEvent[E] =
    _next[KeyedEvent[E]](e ⇒ (implicitClass[E] isAssignableFrom e.event.getClass) && condition(e), timeout)

  private def _next[E <: AnyKeyedEvent : ClassTag](predicate: E ⇒ Boolean = EveryEvent, timeout: Duration = defaultTimeout): E =
    nextEvent(timeout, predicate, implicitClass[E])

  def queued[E <: Event: ClassTag]: immutable.Seq[KeyedEvent[E]] = {
    val result = mutable.ListBuffer[KeyedEvent[E]]()
    try while (true) result += _next[KeyedEvent[E]](e ⇒ implicitClass[E] isAssignableFrom e.event.getClass, 0.s)
    catch { case _: TimeoutException ⇒ }
    result.toList
  }

  private def nextEvent[E <: AnyKeyedEvent](timeout: Duration, predicate: E ⇒ Boolean, expectedEventClass: Class[E]): E = {
    val until = now() + timeout
    def expectedName = expectedEventClass.getSimpleName

    @tailrec def waitForEvent(): E =
      tryPoll(until - now()) match {
        case None ⇒
          throw new TimeoutException(s"Expected event '$expectedName' has not arrived within ${timeout.pretty}")
        case Some(KeyedEvent(_, e: TerminatedEvent)) ⇒
          sys.error(s"Expected event '$expectedName' has not arrived before ${classOf[TerminatedEvent].getName} has arrived")
        case Some(e: AnyKeyedEvent) if (expectedEventClass isAssignableFrom e.getClass) && evalPredicateIfDefined(predicate, e.asInstanceOf[E]) ⇒
          e.asInstanceOf[E]
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

  private def evalPredicateIfDefined[E <: AnyKeyedEvent](predicate: E ⇒ Boolean, event: E): Boolean =
    predicate match {
      case pf: PartialFunction[E, Boolean] ⇒ PartialFunction.cond(event)(pf)
      case _ ⇒ predicate(event)
    }

  def isKeyedEvent[E <: Event: ClassTag](key: E#Key)(event: KeyedEvent[E]) =
    event match {
      case keyedEvent: KeyedEvent[E @unchecked] ⇒ keyedEvent.key == key && implicitClass[E].isAssignableFrom(keyedEvent.event.getClass)
      case _ ⇒ false
    }
}
