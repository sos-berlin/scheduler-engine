package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.base.utils.ScalaUtils.{implicitClass, withToString1}
import com.sos.scheduler.engine.common.scalautil.Futures._
import com.sos.scheduler.engine.data.event.{AnyKeyedEvent, Event, KeyedEvent}
import com.sos.scheduler.engine.eventbus.{EventBus, EventSubscription}
import java.time.Duration
import java.util.concurrent.TimeoutException
import scala.concurrent.{ExecutionContext, Future, Promise}
import scala.reflect.ClassTag

object EventBusTestFutures {

  val EveryEvent: AnyKeyedEvent ⇒ Boolean = _ ⇒ true

  object implicits {
    implicit class RichEventBus(val delegate: EventBus) extends AnyVal {

      def awaiting[E <: Event: ClassTag](key: E#Key)(body: ⇒ Unit)(implicit timeout: ImplicitTimeout): E =
        awaitingInTimeWhen[E](
          timeout = timeout.duration,
          predicate = withToString1(s"'$key'") { _.key == key }
        )(body).event

      def awaitingWhen[E <: Event: ClassTag](predicate: KeyedEvent[E] ⇒ Boolean)(body: ⇒ Unit)(implicit timeout: ImplicitTimeout): KeyedEvent[E] =
        awaitingInTimeWhen[E](timeout.duration, predicate)(body)

      def awaitingInTimeWhen[E <: Event: ClassTag](timeout: Duration, predicate: KeyedEvent[E] ⇒ Boolean)(body: ⇒ Unit): KeyedEvent[E] = {
        val future = keyedEventFutureWhen[E](predicate)
        body
        try awaitResult(future, timeout)
        catch {
          case t: TimeoutException ⇒ throw new TimeoutException(s"${t.getMessage}, while waiting for event ${implicitClass[E].getName} (with $predicate)")
        }
      }

      def eventFuture[E <: Event: ClassTag](key: E#Key): Future[E] =
        keyedEventFutureWhen[E](_.key == key).map { _.event }(SynchronousExecutionContext)

      /** @return Future, will succeed with next [[Event]] `E`. */
      def future[E <: Event: ClassTag](implicit ec: ExecutionContext): Future[E] =
        futureWhen[E](EveryEvent)

      /** @return Future, will succeed with next [[Event]] `E` matching the `predicate`. */
      def futureWhen[E <: Event: ClassTag](predicate: KeyedEvent[E] ⇒ Boolean): Future[E] =
        keyedEventFutureWhen[E](predicate).map { _.event }(SynchronousExecutionContext)

      /** @return Future, will succeed with next [[KeyedEvent]]`[E]` matching the `predicate`. */
      def keyedEventFutureWhen[E <: Event: ClassTag](predicate: KeyedEvent[E] ⇒ Boolean): Future[KeyedEvent[E]] = {
        val promise = Promise[KeyedEvent[E]]()
        lazy val eventSubscription: EventSubscription = EventSubscription[E] {
          case e if predicate(e) ⇒
            promise.trySuccess(e)
            delegate unsubscribe eventSubscription
        }
        delegate subscribe eventSubscription
        promise.future
      }
    }
  }
}
