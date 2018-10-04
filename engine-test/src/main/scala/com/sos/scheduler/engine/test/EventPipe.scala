package com.sos.scheduler.engine.test

import com.google.common.collect.HashMultiset
import com.sos.scheduler.engine.common.scalautil.AutoClosing.closeOnError
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.implicitClass
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.event.{Event, KeyedEvent}
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

  private val queue = new LinkedBlockingQueue[Event]

  closeOnError(closer) {
    eventBus.on[Event] { case e ⇒ queue.add(e) }
  }

  def nextKeyedEvents[E <: KeyedEvent : ClassTag](keys: Iterable[E#Key]): immutable.Seq[E] = {
    val remainingKeys = HashMultiset.create(asJavaIterable(keys))
    val events = mutable.Map[E#Key, E]()
    while (!remainingKeys.isEmpty) {
      val e = nextAny[E]
      if (remainingKeys.remove(e.key)) {
        events += e.key → e
      }
    }
    keys.map(events).toVector
  }

  def nextAny[E <: Event : ClassTag]: E = next[E]()

  def nextKeyed[E <: KeyedEvent : ClassTag](key: E#Key, timeout: Duration = defaultTimeout): E =
    next[E]({ e: E ⇒ e.key == key }, timeout)

  def nextWithCondition[E <: Event : ClassTag](condition: E ⇒ Boolean): E =
    next[E](condition, defaultTimeout)

  def nextWithTimeoutAndCondition[E <: Event : ClassTag](timeout: Duration)(condition: E ⇒ Boolean): E =
    next[E](condition, timeout)

  def next[E <: Event : ClassTag](predicate: E ⇒ Boolean = EveryEvent, timeout: Duration = defaultTimeout): E =
    nextEvent2(timeout, predicate, implicitClass[E])

  /**
   * @return All so far queued events of type E.
   */
  def queued[E <: Event : ClassTag]: immutable.Seq[E] = {
    val result = mutable.ListBuffer[E]()
    try while (true) result += nextEvent2(0.s, EveryEvent, implicitClass[E])
    catch { case _: TimeoutException ⇒ }
    result.toList
  }

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

  def poll(t: Duration): Event =
    tryPoll(t) getOrElse sys.error(s"Event has not arrived within ${t.pretty}")

  def tryPoll(t: Duration): Option[Event] =
    Option(queue.poll(t.toMillis, TimeUnit.MILLISECONDS))
}

object EventPipe {
  private val EveryEvent = (_: Event) ⇒ true

  class TimeoutException(override val getMessage: String) extends RuntimeException

  private def evalPredicateIfDefined[E <: Event](predicate: E ⇒ Boolean, event: E): Boolean =
    predicate match {
      case pf: PartialFunction[E, Boolean] ⇒ PartialFunction.cond(event)(pf)
      case _ ⇒ predicate(event)
    }
}
