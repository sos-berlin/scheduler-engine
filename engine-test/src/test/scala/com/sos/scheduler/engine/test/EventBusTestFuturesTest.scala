package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.data.event.{Event, KeyedEvent}
import com.sos.scheduler.engine.eventbus.HotEventBus
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.EventBusTestFuturesTest._
import java.util.concurrent.TimeUnit
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.concurrent.Await
import scala.concurrent.duration.FiniteDuration

@RunWith(classOf[JUnitRunner])
final class EventBusTestFuturesTest extends FreeSpec {
  "eventFuture without predicate" in {
    val eventBus = new HotEventBus
    val future = eventBus.eventFuture[MyEvent]()
    eventBus shouldBe 'subscribed
    future should not be 'completed
    eventBus.publish(MyEvent(42))
    future shouldBe 'completed
    Await.result(future, new FiniteDuration(5, TimeUnit.SECONDS)) shouldEqual MyEvent(42)
    eventBus should not be 'subscribed
  }

  "eventFuture with predicate" in {
    val eventBus = new HotEventBus
    val future = eventBus.eventFuture[MyEvent](predicate = _.i == 42)
    eventBus shouldBe 'subscribed
    future should not be 'completed
    eventBus.publish(MyEvent(7))
    future should not be 'completed
    eventBus.publish(MyEvent(42))
    future shouldBe 'completed
    Await.result(future, new FiniteDuration(5, TimeUnit.SECONDS)) shouldEqual MyEvent(42)
    eventBus should not be 'subscribed
  }

  "eventFuture KeyedEvent" in {
    val eventBus = new HotEventBus
    val future = eventBus.keyedEventFuture[MyKeyedEvent](42)
    eventBus shouldBe 'subscribed
    future should not be 'completed
    eventBus.publish(MyKeyedEvent(42))
    future shouldBe 'completed
    Await.result(future, new FiniteDuration(5, TimeUnit.SECONDS)) shouldEqual MyKeyedEvent(42)
    eventBus should not be 'subscribed
  }
}

private object EventBusTestFuturesTest {
  private case class MyEvent(i: Int) extends Event

  private case class MyKeyedEvent(key: Int) extends KeyedEvent {
    type Key = Int
  }
}
