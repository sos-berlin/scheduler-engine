package com.sos.scheduler.engine.test

import _root_.scala.concurrent.{Promise, Future, Await}
import _root_.scala.reflect.ClassTag
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.event.{Event, KeyedEvent}
import com.sos.scheduler.engine.eventbus.{EventSubscription, EventBus}
import org.joda.time.Duration

object EventBusTestFutures {

  object implicits {
    implicit class RichEventBus(val delegate: EventBus) extends AnyVal {

      def awaitingKeyedEvent[E <: KeyedEvent](key: E#Key)(f: => Unit)(implicit e: ClassTag[E], timeout: TestTimeout) {
        awaitingEvent2[E](predicate = _.key == key, timeout = timeout.duration)(f)(e)
      }

      def awaitingEvent[E <: Event](predicate: E => Boolean = (_: E) => true)(f: => Unit)(implicit e: ClassTag[E], timeout: TestTimeout) {
        awaitingEvent2[E](predicate = predicate, timeout = timeout.duration)(f)(e)
      }

      private def awaitingEvent2[E <: Event](timeout: Duration, predicate: E => Boolean = (_: E) => true)(f: => Unit)(implicit e: ClassTag[E]) {
        val future = eventFuture[E](predicate = predicate)(e)
        f
        Await.result(future, timeout)
      }

      /** @return Future, der mit dem nächsten KeyedEvent E erfolgreich endet. */
      def keyedEventFuture[E <: KeyedEvent](key: E#Key)(implicit e: ClassTag[E]): Future[E] =
        eventFuture[E](predicate = _.key == key)

      /** @return Future, der mit dem nächsten Event E und dem erfüllten Prädikat erfolgreich endet. */
      def eventFuture[E <: Event](predicate: E => Boolean = (_: E) => true)(implicit e: ClassTag[E]): Future[E] = {
        val promise = Promise[E]()
        lazy val eventSubscription: EventSubscription = EventSubscription[E] { e =>
          if (predicate(e)) {
            promise.success(e)
            delegate unregister eventSubscription
          }
        }
        delegate register eventSubscription
        promise.future
      }
    }
  }
}
