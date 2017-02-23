package com.sos.scheduler.engine.test

import com.sos.jobscheduler.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.data.event.{Event, KeyedEvent}
import com.sos.scheduler.engine.eventbus.HotEventBus
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.EventBusTestFuturesTest._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.concurrent.Future

@RunWith(classOf[JUnitRunner])
final class EventBusTestFuturesTest extends FreeSpec {

  "eventFuture without predicate" in {
    val eventBus = new HotEventBus
    val future: Future[MyEvent] = eventBus.eventFuture[MyEvent](1)
    eventBus shouldBe 'subscribed
    future should not be 'completed
    eventBus.publish(KeyedEvent(MyEvent(42))(1))
    future await 1.s
    future shouldBe 'completed
    future await 5.s shouldEqual MyEvent(42)
    eventBus should not be 'subscribed
  }

  "keyedEventFutureWhen" in {
    val eventBus = new HotEventBus
    val future: Future[KeyedEvent[MyEvent]] = eventBus.keyedEventFutureWhen[MyEvent](predicate = _.event.i == 42)
    eventBus shouldBe 'subscribed
    future should not be 'completed
    eventBus.publish(KeyedEvent(MyEvent(7))(1))
    sleep(100.ms)
    future should not be 'completed
    eventBus.publish(KeyedEvent(MyEvent(42))(2))
    future await 1.s
    future shouldBe 'completed
    future await 5.s shouldEqual KeyedEvent(MyEvent(42))(2)
    eventBus should not be 'subscribed
  }

  "awaiting timeout exception contains expected key value" in {
    val eventBus = new HotEventBus
    val key = 123456789
    implicit val timeout = ImplicitTimeout(0.s)
    intercept[java.util.concurrent.TimeoutException] { eventBus.awaiting[TestEvent.type](key) {} }
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
