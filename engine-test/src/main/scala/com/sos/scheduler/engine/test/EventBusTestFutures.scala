package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.common.scalautil.Futures._
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.{implicitClass, withToString1}
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.event.{Event, KeyedEvent}
import com.sos.scheduler.engine.eventbus.{EventBus, EventSubscription}
import java.util.concurrent.TimeoutException
import java.time.Duration
import scala.concurrent.{Future, Promise}
import scala.reflect.ClassTag

object EventBusTestFutures {

  val EveryEvent: Event ⇒ Boolean = _ ⇒ true

  object implicits {
    implicit class RichEventBus(val delegate: EventBus) extends AnyVal {

      def awaitingKeyedEvent[E <: KeyedEvent](key: E#Key)(f: ⇒ Unit)(implicit e: ClassTag[E], timeout: ImplicitTimeout): E =
        awaitingEvent2[E](predicate = withToString1(s"'$key'") { _.key == key }, timeout = timeout.duration)(f)(e)

      def awaitingEvent[E <: Event](predicate: E ⇒ Boolean = EveryEvent)(f: ⇒ Unit)(implicit e: ClassTag[E], timeout: ImplicitTimeout): E =
        awaitingEvent2[E](predicate = predicate, timeout = timeout.duration)(f)(e)

      def awaitingEvent2[E <: Event](timeout: Duration, predicate: E ⇒ Boolean = EveryEvent)(f: ⇒ Unit)(implicit e: ClassTag[E]): E = {
        val future = eventFuture[E](predicate = predicate)(e)
        f
        try awaitResult(future, timeout)
        catch {
          case t: TimeoutException ⇒ throw new TimeoutException(s"${t.getMessage}, while waiting for event ${implicitClass[E].getName} (with $predicate)")
        }
      }

      /** @return Future, will succeed with next [[KeyedEvent]] `E`. */
      def keyedEventFuture[E <: KeyedEvent](key: E#Key)(implicit e: ClassTag[E]): Future[E] =
        eventFuture[E](predicate = _.key == key)

      /** @return Future, will succeed with next [[Event]] `E`  matching the `predicate`. */
      def eventFuture[E <: Event](predicate: E ⇒ Boolean = EveryEvent)(implicit e: ClassTag[E]): Future[E] = {
        val promise = Promise[E]()
        lazy val eventSubscription: EventSubscription = EventSubscription[E] { e ⇒
          if (predicate(e)) {
            promise.trySuccess(e)
            delegate unregister eventSubscription
          }
        }
        delegate register eventSubscription
        promise.future
      }
    }
  }
}
