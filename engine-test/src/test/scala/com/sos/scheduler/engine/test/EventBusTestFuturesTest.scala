package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.event.{KeyedEvent, Event}
import com.sos.scheduler.engine.eventbus.HotEventBus
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.EventBusTestFuturesTest._
import java.util.concurrent.TimeUnit.SECONDS
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.duration.FiniteDuration
import scala.concurrent.{Await, Future}

@RunWith(classOf[JUnitRunner])
final class EventBusTestFuturesTest extends FreeSpec {

  "keyedEventFuture without predicate" in {
    val eventBus = new HotEventBus
    val future: Future[MyEvent] = eventBus.keyedEventFuture[MyEvent](1)
    eventBus shouldBe 'subscribed
    future should not be 'completed
    eventBus.publish(KeyedEvent(MyEvent(42))(1))
    future await 1.s
    future shouldBe 'completed
    Await.result(future, new FiniteDuration(5, SECONDS)) shouldEqual MyEvent(42)
    eventBus should not be 'subscribed
  }

  "eventFuture with predicate" in {
    val eventBus = new HotEventBus
    val future: Future[MyEvent] = eventBus.eventFuture[MyEvent](predicate = _.event.i == 42)
    eventBus shouldBe 'subscribed
    future should not be 'completed
    eventBus.publish(KeyedEvent(MyEvent(7))(1))
    sleep(100.ms)
    future should not be 'completed
    eventBus.publish(KeyedEvent(MyEvent(42))(2))
    future await 1.s
    future shouldBe 'completed
    Await.result(future, new FiniteDuration(5, SECONDS)) shouldEqual MyEvent(42)
    eventBus should not be 'subscribed
  }

  "awaitingKeyedEvent timeout exception contains expected key value" in {
    val eventBus = new HotEventBus
    val key = 123456789
    implicit val timeout = ImplicitTimeout(0.s)
    intercept[java.util.concurrent.TimeoutException] { eventBus.awaitingKeyedEvent[TestEvent.type](key) {} }
      .toString should include (key.toString)
  }
}

private object EventBusTestFuturesTest {
  private case class MyEvent(i: Int) extends Event {
    type Key = Int
  }

  private case object TestEvent extends Event {
    type Key = Int
  }
}
