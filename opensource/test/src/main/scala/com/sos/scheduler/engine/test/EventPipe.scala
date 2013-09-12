package com.sos.scheduler.engine.test

import EventPipe._
import _root_.scala.annotation.tailrec
import _root_.scala.reflect.ClassTag
import _root_.scala.sys.error
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.utils.SosAutoCloseable
import com.sos.scheduler.engine.data.event.{KeyedEvent, Event}
import com.sos.scheduler.engine.eventbus._
import com.sos.scheduler.engine.main.event.TerminatedEvent
import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.TimeUnit
import org.joda.time.Instant.now
import org.joda.time.{ReadableDuration, Duration}

final class EventPipe(eventBus: EventBus, defaultTimeout: Duration)
extends EventHandlerAnnotated with SosAutoCloseable {

  private val queue = new LinkedBlockingQueue[Event]

  def close() {
    eventBus unregisterAnnotated this
  }

  @EventHandler def add(e: Event) {
    queue.add(e)
  }

  def nextAny[E <: Event : ClassTag]: E =
    nextEvent[E](defaultTimeout, everyEvent)

  def nextKeyed[E <: KeyedEvent : ClassTag](key: E#Key, timeout: Duration = defaultTimeout): E =
    nextEvent[E](timeout, { _.key == key })

  def nextWithCondition[E <: Event : ClassTag](condition: E => Boolean): E =
    nextEvent[E](defaultTimeout, condition)

  def nextWithTimeoutAndCondition[E <: Event : ClassTag](timeout: Duration)(condition: E => Boolean): E =
    nextEvent[E](timeout, condition)

  private def nextEvent[E <: Event](timeout: Duration, predicate: E => Boolean)(implicit classTag: ClassTag[E]): E =
    nextEvent2(timeout, predicate, classTag.runtimeClass.asInstanceOf[Class[E]])

  private def nextEvent2[E <: Event](timeout: Duration, predicate: E => Boolean, expectedEventClass: Class[E]): E = {
    val until = now() + timeout

    @tailrec def waitForEvent(): E = {
      def expectedName = expectedEventClass.getSimpleName
      tryPoll(until - now()) match {
        case None => throw new TimeoutException(s"Expected Event '$expectedName' has not arrived with ${timeout.pretty}")
        case Some(e: TerminatedEvent) => error(s"Expected event '$expectedName' has not arrived before ${classOf[TerminatedEvent].getName} has arrived")
        case Some(e: E) if (expectedEventClass isAssignableFrom e.getClass) && evalPredicateIfDefined(predicate, e) => e
        case _ => waitForEvent()
      }
    }

    waitForEvent()
  }

  def poll(t: ReadableDuration): Event =
    tryPoll(t) getOrElse error(s"Event has not arrived within ${t.pretty}")

  def tryPoll(t: ReadableDuration): Option[Event] =
    Option(queue.poll(t.getMillis, TimeUnit.MILLISECONDS))
}

object EventPipe {
  private val everyEvent = (e: Event) => true

  class TimeoutException(override val getMessage: String) extends RuntimeException

  private def evalPredicateIfDefined[E <: Event](o: E => Boolean, e: E) = o match {
    case o: PartialFunction[Event, Boolean] => (o isDefinedAt e) && o(e)
    case _ => o(e)
  }
}
