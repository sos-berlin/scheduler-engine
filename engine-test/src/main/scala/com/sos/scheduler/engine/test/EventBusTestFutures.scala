package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.base.utils.ScalaUtils.{implicitClass, withToString1}
import com.sos.scheduler.engine.common.scalautil.Futures._
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, KeyedEvent, Event}
import com.sos.scheduler.engine.eventbus.{EventBus, EventSubscription}
import com.sos.scheduler.engine.test.EventPipe.isKeyedEvent
import java.time.Duration
import java.util.concurrent.TimeoutException
import scala.concurrent.{ExecutionContext, Future, Promise}
import scala.reflect.ClassTag

object EventBusTestFutures {

  val EveryEvent: AnyKeyedEvent ⇒ Boolean = _ ⇒ true

  object implicits {
    implicit class RichEventBus(val delegate: EventBus) extends AnyVal {

      def awaitingKeyedEvent[E <: Event: ClassTag](key: E#Key)(f: ⇒ Unit)(implicit timeout: ImplicitTimeout): E =
        _awaitingEvent[KeyedEvent[E]](
          predicate = withToString1(s"'$key'") { isKeyedEvent[E](key) },
          timeout = timeout.duration
        )(f).event

      def awaitingEvent[E <: Event: ClassTag](predicate: KeyedEvent[E] ⇒ Boolean)(f: ⇒ Unit)(implicit timeout: ImplicitTimeout): E =
        awaitingEvent2[E](timeout.duration, predicate)(f)

      def awaitingEvent2[E <: Event: ClassTag](timeout: Duration, predicate: KeyedEvent[E] ⇒ Boolean)(f: ⇒ Unit): E =
        _awaitingEvent[KeyedEvent[E]](
          predicate = e ⇒ (implicitClass[E] isAssignableFrom e.event.getClass) && predicate(e),
          timeout = timeout
        )(f).event

      private def _awaitingEvent[E <: AnyKeyedEvent](timeout: Duration, predicate: E ⇒ Boolean = EveryEvent)(f: ⇒ Unit)(implicit e: ClassTag[E]): E = {
        val future = _eventFuture[E](predicate = predicate)(e)
        f
        try awaitResult(future, timeout)
        catch {
          case t: TimeoutException ⇒ throw new TimeoutException(s"${t.getMessage}, while waiting for event ${implicitClass[E].getName} (with $predicate)")
        }
      }

      def keyedEventFuture[E <: Event: ClassTag](key: E#Key)(implicit ec: ExecutionContext): Future[E] =
        _eventFuture[KeyedEvent[E]](isKeyedEvent[E](key)) map { _.event }

      /** @return Future, will succeed with next [[KeyedEvent[E]]. */
      def eventFuture[E <: Event: ClassTag](predicate: KeyedEvent[E] ⇒ Boolean)(implicit ec: ExecutionContext): Future[E] =
        _eventFuture[KeyedEvent[E]](e ⇒ (implicitClass[E] isAssignableFrom e.event.getClass) && predicate(e)) map { _.event }

      /** @return Future, will succeed with next [[Event]] `E` matching the `predicate`. */
      private def _eventFuture[E <: AnyKeyedEvent: ClassTag](predicate: E ⇒ Boolean = EveryEvent): Future[E] = {
        val promise = Promise[E]()
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
