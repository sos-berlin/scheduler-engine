package com.sos.scheduler.engine.tests.jira.js1659

import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.event.{Event, EventSeq, KeyedEvent, Snapshot}
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.order.OrderNodeTransition.Success
import com.sos.scheduler.engine.data.order.{OrderNodeChanged, OrderNodeTransition, OrderStarted, OrderStepEnded, OrderStepStarted, OrderSuspended}
import com.sos.scheduler.engine.data.xmlcommands.{ModifyOrderCommand, OrderCommand}
import com.sos.scheduler.engine.kernel.event.collector.EventIdGenerator
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1659.JS1659IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.collection.immutable.Seq

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1659IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val httpPort = findRandomFreeTcpPort()
  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-http-port=127.0.0.1:$httpPort"))
  private val client = new StandardWebSchedulerClient(s"http://127.0.0.1:$httpPort").closeWithCloser
  private lazy val eventIdGenerator = instance[EventIdGenerator]

  "EventSeq.NonEmpty" in {
    val beforeTestEventId = eventIdGenerator.lastUsedEventId
    eventBus.awaiting[OrderSuspended.type](TestOrderKey) {
      scheduler executeXml OrderCommand(TestOrderKey)
    }
    val Snapshot(_, EventSeq.NonEmpty(snapshots)) = client.events[Event](after = beforeTestEventId) await TestTimeout
    val events: Seq[KeyedEvent[Event]] = snapshots map { _.value }
    assert(events == Seq(
      KeyedEvent(OrderStarted)(TestOrderKey),
      KeyedEvent(OrderStepStarted(NodeId("100"), TaskId.First))(TestOrderKey),
      KeyedEvent(OrderStepEnded(OrderNodeTransition.Success))(TestOrderKey),
      KeyedEvent(OrderNodeChanged(NodeId("200"), NodeId("100")))(TestOrderKey),
      KeyedEvent(OrderStepStarted(NodeId("200"), TaskId.First))(TestOrderKey),
      KeyedEvent(OrderStepEnded(Success))(TestOrderKey),
      KeyedEvent(OrderSuspended)(TestOrderKey),
      KeyedEvent(OrderNodeChanged(NodeId("300"), NodeId("200")))(TestOrderKey)))
  }

  "EventSeq.Teared" in {
    val oldEventId = eventIdGenerator.lastUsedEventId
    // Generate enough events to tear the event stream starting at beforeTestEventId
    for (_ ← 1 to 10) {
      scheduler executeXml ModifyOrderCommand(TestOrderKey, action = Some(ModifyOrderCommand.Action.reset))
      eventBus.awaiting[OrderSuspended.type](TestOrderKey) {
        scheduler executeXml OrderCommand(TestOrderKey)
      }
    }
    val snapshot = client.events[Event](after = oldEventId) await TestTimeout
    assert(snapshot.value == EventSeq.Teared)
  }
}

private object JS1659IT {
  private val TestOrderKey = JobChainPath("/test") orderKey "1"
}
