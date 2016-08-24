package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.base.utils.ScalaUtils.{implicitClass, withToString1}
import com.sos.scheduler.engine.common.scalautil.Futures._
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, KeyedEvent, Event}
import com.sos.scheduler.engine.eventbus.{EventBus, EventSubscription}
import java.time.Duration
import java.util.concurrent.TimeoutException
import scala.concurrent.{ExecutionContext, Future, Promise}
import scala.reflect.ClassTag

object EventBusTestFutures {

  val EveryEvent: AnyKeyedEvent ⇒ Boolean = _ ⇒ true

  object implicits {
    implicit class RichEventBus(val delegate: EventBus) extends AnyVal {

      def awaitingKeyedEvent[E <: Event: ClassTag](key: E#Key)(body: ⇒ Unit)(implicit timeout: ImplicitTimeout): E =
        _awaitingEvent[E](
          predicate = withToString1(s"'$key'") { _.key == key },
          timeout = timeout.duration
        )(body).event

      def awaitingEvent[E <: Event: ClassTag](predicate: KeyedEvent[E] ⇒ Boolean)(body: ⇒ Unit)(implicit timeout: ImplicitTimeout): E =
        awaitingEvent2[E](timeout.duration, predicate)(body)

      def awaitingEvent2[E <: Event: ClassTag](timeout: Duration, predicate: KeyedEvent[E] ⇒ Boolean)(body: ⇒ Unit): E =
        _awaitingEvent[E](timeout, predicate)(body).event

      private def _awaitingEvent[E <: Event: ClassTag](timeout: Duration, predicate: KeyedEvent[E] ⇒ Boolean = EveryEvent)(body: ⇒ Unit): KeyedEvent[E] = {
        val future = _eventFuture[E](predicate = predicate)
        body
        try awaitResult(future, timeout)
        catch {
          case t: TimeoutException ⇒ throw new TimeoutException(s"${t.getMessage}, while waiting for event ${implicitClass[E].getName} (with $predicate)")
        }
      }

      def keyedEventFuture[E <: Event: ClassTag](key: E#Key)(implicit ec: ExecutionContext): Future[E] =
        _eventFuture[E](_.key == key) map { _.event }

      /** @return Future, will succeed with next [[KeyedEvent[E]]. */
      def eventFuture[E <: Event: ClassTag](predicate: KeyedEvent[E] ⇒ Boolean)(implicit ec: ExecutionContext): Future[E] =
        _eventFuture[E](predicate) map { _.event }

      /** @return Future, will succeed with next [[Event]] `E` matching the `predicate`. */
      private def _eventFuture[E <: Event: ClassTag](predicate: KeyedEvent[E] ⇒ Boolean = EveryEvent): Future[KeyedEvent[E]] = {
        val promise = Promise[KeyedEvent[E]]()
        lazy val eventSubscription: EventSubscription = EventSubscription[E] { case e ⇒
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
