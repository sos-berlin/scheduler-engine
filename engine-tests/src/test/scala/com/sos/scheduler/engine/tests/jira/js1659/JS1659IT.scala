package com.sos.scheduler.engine.tests.jira.js1659

import com.sos.scheduler.engine.client.web.StandardWebSchedulerClient
import com.sos.scheduler.engine.common.event.EventIdGenerator
import com.sos.scheduler.engine.common.event.collector.EventCollector
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.scheduler.engine.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.time.WaitForCondition.waitForCondition
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.event.{Event, EventId, EventRequest, EventSeq, KeyedEvent, Snapshot}
import com.sos.scheduler.engine.data.folder.FolderPath
import com.sos.scheduler.engine.data.job.{JobEvent, JobPath, JobState, JobStateChanged, JobUnstopped, ReturnCode, TaskClosed, TaskEnded, TaskEvent, TaskId, TaskKey, TaskStarted}
import com.sos.scheduler.engine.data.jobchain.{JobChainNodeAction, JobChainPath, JobChainState, NodeId}
import com.sos.scheduler.engine.data.order.OrderNodeTransition.Success
import com.sos.scheduler.engine.data.order.{JobChainEvent, JobChainNodeActionChanged, JobChainStateChanged, OrderAdded, OrderNodeChanged, OrderNodeTransition, OrderStarted, OrderStepEnded, OrderStepStarted, OrderSuspended}
import com.sos.scheduler.engine.data.queries.PathQuery
import com.sos.scheduler.engine.data.scheduler.{SchedulerEvent, SchedulerInitiated, SchedulerState, SchedulerStateChanged}
import com.sos.scheduler.engine.data.xmlcommands.{ModifyJobCommand, ModifyOrderCommand, OrderCommand}
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils.jobOverview
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
  private lazy val client = new StandardWebSchedulerClient(s"http://127.0.0.1:$httpPort").closeWithCloser
  private lazy val eventIdGenerator = instance[EventIdGenerator]
  private lazy val beforeTestEventId = eventIdGenerator.lastUsedEventId

  override protected def checkedBeforeAll() = {
    eventBus.onHot[SchedulerInitiated.type] {
      case _ ⇒ instance[EventCollector]  // Start collection of events before Scheduler, to have the very first events available.
    }
    super.checkedBeforeAll()
  }

  "SchedulerEvent" in {
    val Snapshot(_, EventSeq.NonEmpty(eventSnapshots)) = client.events(EventRequest.singleClass[SchedulerEvent](after = EventId.BeforeFirst, TestTimeout)) await TestTimeout
    val events: Seq[KeyedEvent[SchedulerEvent]] = eventSnapshots map { _.value }
    assert(events == List(
      KeyedEvent(SchedulerStateChanged(SchedulerState.starting)),
      KeyedEvent(SchedulerStateChanged(SchedulerState.running))))
  }

  "/api/event EventSeq.NonEmpty" in {
    beforeTestEventId
    eventBus.awaiting[OrderSuspended.type](TestOrderKey) {
      // Some JobChainEvent
      scheduler executeXml <job_chain.modify job_chain={TestJobChainPath} state="stopped"/>
      scheduler executeXml <job_chain.modify job_chain={TestJobChainPath} state="running"/>
      scheduler executeXml <job_chain_node.modify job_chain={TestJobChainPath} state="100" action="next_state"/>
      scheduler executeXml <job_chain_node.modify job_chain={TestJobChainPath} state="100" action="process"/>
      scheduler executeXml OrderCommand(TestOrderKey)
    }
    waitForCondition(10.s, 10.ms)(jobOverview(TestJobPath).state == JobState.pending)
    val Snapshot(aEventId, EventSeq.NonEmpty(eventSnapshots)) = client.events(EventRequest.singleClass[Event](after = beforeTestEventId, TestTimeout)) await TestTimeout
    assert(aEventId >= eventSnapshots.last.eventId)
    val events: Seq[KeyedEvent[Event]] = eventSnapshots map { _.value }
    assert(events == Seq(
      KeyedEvent(JobChainStateChanged(JobChainState.stopped))(TestJobChainPath),
      KeyedEvent(JobChainStateChanged(JobChainState.running))(TestJobChainPath),
      KeyedEvent(JobChainNodeActionChanged(NodeId("100"), JobChainNodeAction.next_state))(TestJobChainPath),
      KeyedEvent(JobChainNodeActionChanged(NodeId("100"), JobChainNodeAction.process))(TestJobChainPath),
      KeyedEvent(OrderAdded(NodeId("100")))(TestOrderKey),
      KeyedEvent(OrderStarted)(TestOrderKey),
      KeyedEvent(JobStateChanged(JobState.running))(TestJobPath),

      KeyedEvent(TaskStarted)(TaskKey1),
      KeyedEvent(OrderStepStarted(NodeId("100"), TaskKey1.taskId))(TestOrderKey),
      KeyedEvent(OrderStepEnded(OrderNodeTransition.Success))(TestOrderKey),
      KeyedEvent(OrderNodeChanged(NodeId("200"), NodeId("100")))(TestOrderKey),
      KeyedEvent(TaskEnded(ReturnCode(0)))(TaskKey1),
      KeyedEvent(TaskClosed)(TaskKey1),

      KeyedEvent(JobStateChanged(JobState.pending))(TestJobPath),
      KeyedEvent(JobStateChanged(JobState.running))(TestJobPath),

      KeyedEvent(TaskStarted)(TaskKey2),
      KeyedEvent(OrderStepStarted(NodeId("200"), TaskKey2.taskId))(TestOrderKey),
      KeyedEvent(OrderStepEnded(Success))(TestOrderKey),
      KeyedEvent(OrderSuspended)(TestOrderKey),
      KeyedEvent(OrderNodeChanged(NodeId("300"), NodeId("200")))(TestOrderKey),
      KeyedEvent(TaskEnded(ReturnCode(0)))(TaskKey2),
      KeyedEvent(TaskClosed)(TaskKey2),

      KeyedEvent(JobStateChanged(JobState.pending))(TestJobPath)))
  }

  "/api/jobChain/" in {
    val eventRequest = EventRequest.singleClass[JobChainEvent](after = beforeTestEventId, 0.s)
    val Snapshot(aEventId, EventSeq.NonEmpty(eventSnapshots)) =
      client.jobChainEventsBy(PathQuery(FolderPath.Root), eventRequest) await TestTimeout
    assert(aEventId >= eventSnapshots.last.eventId)
    val events: Seq[KeyedEvent[Event]] = eventSnapshots map { _.value }
    assert(events == Seq(
      KeyedEvent(JobChainStateChanged(JobChainState.stopped))(TestJobChainPath),
      KeyedEvent(JobChainStateChanged(JobChainState.running))(TestJobChainPath),
      KeyedEvent(JobChainNodeActionChanged(NodeId("100"), JobChainNodeAction.next_state))(TestJobChainPath),
      KeyedEvent(JobChainNodeActionChanged(NodeId("100"), JobChainNodeAction.process))(TestJobChainPath)))
  }

  "/api/jobChain/test" in {
    val eventRequest = EventRequest.singleClass[JobChainEvent](after = beforeTestEventId, 0.s)
    val Snapshot(aEventId, EventSeq.NonEmpty(eventSnapshots)) =
      client.jobChainEvents(TestJobChainPath, eventRequest) await TestTimeout
    assert(aEventId >= eventSnapshots.last.eventId)
    val events: Seq[Event] = eventSnapshots map { _.value }
    assert(events == Seq(
      JobChainStateChanged(JobChainState.stopped),
      JobChainStateChanged(JobChainState.running),
      JobChainNodeActionChanged(NodeId("100"), JobChainNodeAction.next_state),
      JobChainNodeActionChanged(NodeId("100"), JobChainNodeAction.process)))
  }

  "/api/job/" in {
    val eventRequest = EventRequest.singleClass[JobEvent](after = beforeTestEventId, 0.s)
    val Snapshot(aEventId, EventSeq.NonEmpty(eventSnapshots)) =
      client.jobEventsBy(FolderPath.Root, eventRequest) await TestTimeout
    assert(aEventId >= eventSnapshots.last.eventId)
    val events: Seq[KeyedEvent[Event]] = eventSnapshots map { _.value }
    assert(events == Seq(
      KeyedEvent(JobStateChanged(JobState.running))(TestJobPath),
      KeyedEvent(JobStateChanged(JobState.pending))(TestJobPath),
      KeyedEvent(JobStateChanged(JobState.running))(TestJobPath),
      KeyedEvent(JobStateChanged(JobState.pending))(TestJobPath)))
  }

  "/api/job/test" in {
    scheduler executeXml ModifyJobCommand(TestJobPath, cmd = Some(ModifyJobCommand.Cmd.Stop))
    scheduler executeXml ModifyJobCommand(TestJobPath, cmd = Some(ModifyJobCommand.Cmd.Unstop))
    val eventRequest = EventRequest.singleClass[JobEvent](after = beforeTestEventId, 0.s)
    val Snapshot(aEventId, EventSeq.NonEmpty(eventSnapshots)) =
      client.jobEvents(TestJobPath, eventRequest) await TestTimeout
    assert(aEventId >= eventSnapshots.last.eventId)
    val events: Seq[Event] = eventSnapshots map { _.value }
    assert(events == Seq(
      JobStateChanged(JobState.running),
      JobStateChanged(JobState.pending),
      JobStateChanged(JobState.running),
      JobStateChanged(JobState.pending),
      JobStateChanged(JobState.stopped),
      JobStateChanged(JobState.pending),
      JobUnstopped))
  }

  "/api/task/" in {
    val Snapshot(aEventId, EventSeq.NonEmpty(eventSnapshots)) =
      client.taskEventsBy(PathQuery.All, EventRequest.singleClass[TaskEvent](after = beforeTestEventId, 0.s)) await TestTimeout
    assert(aEventId >= eventSnapshots.last.eventId)
    val events: Seq[KeyedEvent[TaskEvent]] = eventSnapshots map { _.value }
    assert(events == Seq(
      KeyedEvent(TaskStarted)(TaskKey1),
      KeyedEvent(TaskEnded(ReturnCode(0)))(TaskKey1),
      KeyedEvent(TaskClosed)(TaskKey1),
      KeyedEvent(TaskStarted)(TaskKey2),
      KeyedEvent(TaskEnded(ReturnCode(0)))(TaskKey2),
      KeyedEvent(TaskClosed)(TaskKey2)))
  }

  "/api/task&taskId=3" in {
    val Snapshot(aEventId, EventSeq.NonEmpty(eventSnapshots)) =
      client.taskEvents(TaskKey1.taskId, EventRequest.singleClass[TaskEvent](after = beforeTestEventId, 0.s)) await TestTimeout
    assert(aEventId >= eventSnapshots.last.eventId)
    val events: Seq[TaskEvent] = eventSnapshots map { _.value }
    assert(events == Seq(
      TaskStarted,
      TaskEnded(ReturnCode(0)),
      TaskClosed))
  }

  "EventSeq.Torn" in {
    val oldEventId = eventIdGenerator.lastUsedEventId
    // Generate enough events to tear the event stream starting at beforeTestEventId
    for (_ ← 1 to 3) {
      scheduler executeXml ModifyOrderCommand(TestOrderKey, action = Some(ModifyOrderCommand.Action.reset))
      eventBus.awaiting[OrderSuspended.type](TestOrderKey) {
        scheduler executeXml OrderCommand(TestOrderKey)
      }
    }
    val Snapshot(eventId, eventSeq) = client.events(EventRequest.singleClass[Event](after = oldEventId, TestTimeout)) await TestTimeout
    assert(eventSeq == EventSeq.Torn)
    assert(eventId >= oldEventId)
  }
}

private object JS1659IT {
  private val TestJobChainPath = JobChainPath("/test")
  private val TestOrderKey = TestJobChainPath orderKey "1"
  private val TestJobPath = JobPath("/test")
  private val TaskKey1 = TaskKey(TestJobPath, TaskId.First)
  private val TaskKey2 = TaskKey(TestJobPath, TaskId.First + 1)
}
