package com.sos.scheduler.engine.test

import EventPipe._
import _root_.scala.annotation.tailrec
import _root_.scala.reflect.ClassTag
import _root_.scala.sys.error
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.utils.SosAutoCloseable
import com.sos.scheduler.engine.data.event.Event
import com.sos.scheduler.engine.eventbus._
import com.sos.scheduler.engine.main.event.TerminatedEvent
import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.TimeUnit
import org.joda.time.Instant.now
import org.joda.time.{ReadableDuration, Instant, Duration}

class EventPipe(eventBus: EventBus, defaultTimeout: Duration)
extends EventHandlerAnnotated with SosAutoCloseable {

  private final val queue = new LinkedBlockingQueue[Event]

  def close() {
    eventBus unregisterAnnotated this
  }

  @EventHandler def add(e: Event) {
    queue.add(e)
  }

  def next[E <: Event](implicit c: ClassTag[E]): E =
    nextEvent[E](now() + defaultTimeout, everyEvent, c)

  def nextWithCondition[E <: Event](condition: E => Boolean = everyEvent)(implicit c: ClassTag[E]) =
    nextEvent[E](now() + defaultTimeout, condition, c)

  def nextWithTimeoutAndCondition[E <: Event](timeout: Duration)(condition: E => Boolean = everyEvent)(implicit c: ClassTag[E]) =
    nextEvent[E](now() + timeout, condition, c)

  private def nextEvent[E <: Event](until: Instant, predicate: E => Boolean, classTag: ClassTag[E]): E =
    nextEvent[E](until, predicate, classTag.runtimeClass.asInstanceOf[Class[E]])

  @tailrec private def nextEvent[E <: Event](until: Instant, predicate: E => Boolean, expectedEventClass: Class[E]): E = {
    def expectedName = expectedEventClass.getSimpleName
    tryPoll(until - now()) match {
      case None => throw new TimeoutException(s"Expected Event '$expectedName' has not arrived until $until")
      case Some(e: TerminatedEvent) => error(s"Expected event '$expectedName' has not arrived before ${classOf[TerminatedEvent].getName} has arrived")
      case Some(e: E) if (expectedEventClass isAssignableFrom e.getClass) && predicate(e) => e
      case _ => nextEvent[E](until, predicate, expectedEventClass)
    }
  }

  def poll(t: ReadableDuration): Event =
    tryPoll(t) getOrElse error(s"Event has not arrived within $t")

  def tryPoll(t: ReadableDuration): Option[Event] =
    Option(queue.poll(t.getMillis, TimeUnit.MILLISECONDS))
}

object EventPipe {
  private val everyEvent = (e: Event) => true

  class TimeoutException(override val getMessage: String) extends RuntimeException
}
