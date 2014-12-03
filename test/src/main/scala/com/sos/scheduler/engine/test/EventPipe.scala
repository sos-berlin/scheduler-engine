package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.common.scalautil.ScalaUtils.implicitClass
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.event.{Event, KeyedEvent}
import com.sos.scheduler.engine.eventbus._
import com.sos.scheduler.engine.main.event.TerminatedEvent
import com.sos.scheduler.engine.test.EventPipe._
import java.util.concurrent.{LinkedBlockingQueue, TimeUnit}
import org.joda.time.Instant.now
import org.joda.time.{Duration, ReadableDuration}
import scala.annotation.tailrec
import scala.collection.{immutable, mutable}
import scala.reflect.ClassTag

final class EventPipe(eventBus: EventBus, defaultTimeout: Duration)
extends EventHandlerAnnotated with AutoCloseable {

  private val queue = new LinkedBlockingQueue[Event]

  def close(): Unit = {
    eventBus unregisterAnnotated this
  }

  @EventHandler def add(e: Event): Unit = {
    queue.add(e)
  }

  def nextAny[E <: Event : ClassTag]: E =
    nextEvent[E](defaultTimeout, everyEvent)

  def nextKeyed[E <: KeyedEvent : ClassTag](key: E#Key, timeout: Duration = defaultTimeout): E =
    nextEvent[E](timeout, { _.key == key })

  def nextWithCondition[E <: Event : ClassTag](condition: E ⇒ Boolean): E =
    nextEvent[E](defaultTimeout, condition)

  def nextWithTimeoutAndCondition[E <: Event : ClassTag](timeout: Duration)(condition: E ⇒ Boolean): E =
    nextEvent[E](timeout, condition)

  /** @return All so far queued events of type E. */
  def queued[E <: Event : ClassTag]: immutable.Seq[E] = {
    val result = mutable.ListBuffer[E]()
    try while (true) result += nextEvent2(0.s, everyEvent, implicitClass[E])
    catch { case _: TimeoutException ⇒ }
    result.toList
  }

  private def nextEvent[E <: Event : ClassTag](timeout: Duration, predicate: E ⇒ Boolean): E =
    nextEvent2(timeout, predicate, implicitClass[E])

  private def nextEvent2[E <: Event](timeout: Duration, predicate: E ⇒ Boolean, expectedEventClass: Class[E]): E = {
    val until = now() + timeout
    def expectedName = expectedEventClass.getSimpleName

    @tailrec def waitForEvent(): E =
      tryPoll(until - now()) match {
        case None ⇒
          throw new TimeoutException(s"Expected event '$expectedName' has not arrived within ${timeout.pretty}")
        case Some(e: TerminatedEvent) ⇒
          sys.error(s"Expected event '$expectedName' has not arrived before ${classOf[TerminatedEvent].getName} has arrived")
        case Some(e: Event) if (expectedEventClass isAssignableFrom e.getClass) && evalPredicateIfDefined(predicate, e.asInstanceOf[E]) ⇒
          e.asInstanceOf[E]
        case _ ⇒
          waitForEvent()
      }

    waitForEvent()
  }

  def poll(t: ReadableDuration): Event =
    tryPoll(t) getOrElse sys.error(s"Event has not arrived within ${t.pretty}")

  def tryPoll(t: ReadableDuration): Option[Event] =
    Option(queue.poll(t.getMillis, TimeUnit.MILLISECONDS))
}

object EventPipe {
  private val everyEvent = (e: Event) ⇒ true

  class TimeoutException(override val getMessage: String) extends RuntimeException

  private def evalPredicateIfDefined[E <: Event](predicate: E ⇒ Boolean, event: E): Boolean =
    predicate match {
      case pf: PartialFunction[E, Boolean] ⇒ PartialFunction.cond(event)(pf)
      case _ ⇒ predicate(event)
    }
}
