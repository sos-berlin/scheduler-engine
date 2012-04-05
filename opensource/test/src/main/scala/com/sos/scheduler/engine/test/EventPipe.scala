package com.sos.scheduler.engine.test

import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.TimeUnit
import com.sos.scheduler.engine.main.event.TerminatedEvent
import com.sos.scheduler.engine.eventbus._
import com.sos.scheduler.engine.kernel.util.Time
import com.sos.scheduler.engine.data.event.Event

class EventPipe(defaultTimeout: Time) extends EventHandlerAnnotated {
  private final val queue = new LinkedBlockingQueue[Event]

  @EventHandler def add(e: Event) {
    queue.add(e)
  }

  def next[E <: Event](implicit m: ClassManifest[E]): E = nextEvent[E](defaultTimeout, {e: E => true}, m)

  def nextWithCondition[E <: Event](condition: E => Boolean = {e: E => true})(implicit m: ClassManifest[E]) =
    nextEvent[E](defaultTimeout, condition, m)

  def nextWithTimeoutAndCondition[E <: Event](timeout: Time)(condition: E => Boolean = {e: E => true})(implicit m: ClassManifest[E]) =
    nextEvent[E](timeout, condition, m)

  private def nextEvent[E <: Event](t: Time, predicate: E => Boolean, manifest: ClassManifest[E]): E =
    nextEvent[E](t, predicate, manifest.erasure.asInstanceOf[Class[E]])

  private def nextEvent[E <: Event](t: Time, predicate: E => Boolean, expectedEventClass: Class[E]): E = {
    def expectedName = expectedEventClass.getSimpleName
    tryPoll(t) match {  // TODO besser until - now, jetzt verlängert sich die Frist mit jedem Event
      case None => throw new RuntimeException("Expected Event '"+expectedName+"' has not arrived within "+t)
      case Some(e: TerminatedEvent) => throw new RuntimeException("Expected event '"+expectedName+"' has not arrived before "+classOf[TerminatedEvent].getName+" has arrived")
      case Some(e: E) if (expectedEventClass isAssignableFrom e.getClass) && predicate(e) => e
      case _ => nextEvent[E](t, predicate, expectedEventClass)
    }
  }

  def poll(t: Time): Event = tryPoll(t) getOrElse {throw new RuntimeException("Event has not arrived within "+t)}

  def tryPoll(t: Time) = Option(queue.poll(t.getMillis, TimeUnit.MILLISECONDS))
}
