package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter

import com.sos.scheduler.engine.agent.Agent
import com.sos.scheduler.engine.agent.configuration.AgentConfiguration
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.scheduler.engine.common.scalautil.Collections.emptyToNone
import com.sos.scheduler.engine.common.scalautil.Futures._
import com.sos.scheduler.engine.common.time.{WaitForCondition, TimeoutWithSteps}
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder
import com.sos.scheduler.engine.data.event.Event
import com.sos.scheduler.engine.data.job.{TaskClosedEvent, TaskId}
import com.sos.scheduler.engine.data.log.{LogEvent, SchedulerLogLevel}
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.eventbus.EventSourceEvent
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.kernel.order.{OrderSubsystem, UnmodifiableOrder}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import WaitForCondition.waitForCondition
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.SpoolerProcessAfterIT._
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.expected._
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.setting._
import org.joda.time.Duration.millis
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable
import scala.concurrent.duration._

@RunWith(classOf[JUnitRunner])
final class SpoolerProcessAfterIT extends FreeSpec with ScalaSchedulerTest {

  private lazy val List(tcpPort, javaPort, agentHttpPort) = FreeTcpPortFinder.findRandomFreeTcpPorts(3)

  protected override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    mainArguments = List(
      s"-tcp-port=$tcpPort",
      s"-http-port=$javaPort"),
    terminateOnError = false)

  private lazy val agent = new Agent(AgentConfiguration(httpPort = agentHttpPort, httpInterfaceRestriction = Some("127.0.0.1"))).closeWithCloser
  private val messageCodes = new MyMutableMultiMap[SchedulerLogLevel, String]
  private lazy val jobSubsystem = instance[JobSubsystem]
  private lazy val orderSubsystem = instance[OrderSubsystem]

  private sealed abstract class AgentMode(override val toString: String, val addressOption: () ⇒ Option[String])
  private object NoAgent          extends AgentMode("No Agent"               , () ⇒ None)
  private object JavaAgent        extends AgentMode("Java Agent"             , () ⇒ Some(s"http://127.0.0.1:$agentHttpPort"))
  private object JavaHttpCppAgent extends AgentMode("C++ Agent via Java HTTP", () ⇒ Some(s"http://127.0.0.1:$javaPort"))
  private object CppHttpCppAgent  extends AgentMode("C++ Agent via C++ HTTP" , () ⇒ Some(s"http://127.0.0.1:$tcpPort"))
  private object TcpCppAgent      extends AgentMode("C++ Agent via TCP"      , () ⇒ Some(s"127.0.0.1:$tcpPort"))
  private val allAgentModes = List(NoAgent, JavaAgent, JavaHttpCppAgent, CppHttpCppAgent, TcpCppAgent)

  private val expectedTaskId = Iterator from 3 map TaskId.apply
  Settings.list.zipWithIndex foreach { case ((setting, expected), i) ⇒
    val index = i + 1
    val testName = s"$index. $setting should result in $expected"
    testName - {
      for (mode ← allAgentModes) {
        s"$mode" in {
          def t() = myTest(index, mode, setting, expected, expectedTaskId.next())
          if (mode != NoAgent && index == 2)  // FIXME JS-1330 exit 7 via agent results in order state ERROR instead of InitialState and JobIsStopped
            pendingUntilFixed(t())
          else
            t()
        }
      }
    }
  }

  protected override def onSchedulerActivated() = awaitResult(agent.start(), 10.seconds)

  private def myTest(index: Int, agentMode: AgentMode, setting: Setting, expected: Expected, expectedTaskId: TaskId): Unit =
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      scheduler executeXml <process_class name="test" remote_scheduler={agentMode.addressOption().orNull} replace="true"/>

      val job = jobSubsystem.job(setting.jobPath)

      try {
        val e = execute()
        checkAssertions(e)
      }
      finally cleanUpAfterTest()

      def execute() = {
        scheduler executeXml setting.orderElem
        val result = eventPipe.nextAny[MyFinishedEvent]
        cleanUpAfterExcecute()
        result
      }

      def cleanUpAfterExcecute(): Unit = {
        orderSubsystem.tryRemoveOrder(setting.orderKey)  // Falls Auftrag zurückgestellt ist, damit der Job nicht gleich nochmal mit demselben Auftrag startet.
        job.endTasks()   // Task kann schon beendet und Job schon gestoppt sein.
        eventPipe.nextAny[TaskClosedEvent] match { case e ⇒
          assert(e.taskId == expectedTaskId, "TaskClosedEvent not for expected task - probably a previous test failed")
        }
        waitForCondition(TimeoutWithSteps(millis(3000), millis(10))) { job.state == expected.jobState }   // Der Job-Zustand wird asynchron geändert (stopping -> stopped, running -> pending). Wir warten kurz darauf.
      }

      def checkAssertions(event: MyFinishedEvent): Unit = {
        assert(event.orderKey == setting.orderKey)
        assert(expected.orderStateExpectation matches event.state, s", expected OrderState=${expected.orderStateExpectation}, but was ${event.state}")
        assert(event.spoolerProcessAfterParameterOption == expected.spoolerProcessAfterParameterOption, "Parameter for spooler_process_after(): ")
        assert(job.state == expected.jobState, ", Job.state is not as expected")
        expected.requireMandatoryMessageCodes(messageCodes)
        //??? assert(expected.removeIgnorables(messageCodes).toMap  == expected.messageCodes.toMap)
      }

      def cleanUpAfterTest(): Unit = {
        scheduler executeXml <modify_job job={setting.jobPath.string} cmd="unstop"/>
        messageCodes.clear()
      }
    }

  eventBus.onHotEventSourceEvent[OrderStepEndedEvent] {
    case EventSourceEvent(e: OrderStepEndedEvent, order: UnmodifiableOrder) ⇒
      if (e.stateTransition == KeepOrderStateTransition) {
        // Es wird kein OrderFinishedEvent geben.
        publishMyFinishedEvent(order)
      }
  }

  eventBus.onHotEventSourceEvent[OrderFinishedEvent] {
    case EventSourceEvent(e: OrderFinishedEvent, order: UnmodifiableOrder) ⇒
    publishMyFinishedEvent(order)
  }

  private def publishMyFinishedEvent(order: UnmodifiableOrder): Unit = {
    eventBus.publishCold(MyFinishedEvent(
      order.key, order.state,
      emptyToNone(order.parameters(SpoolerProcessAfterNames.parameter)) map { _.toBoolean }))
  }

  eventBus.on[LogEvent] { case e: LogEvent ⇒
    if (Expected.LogLevels contains e.level) {
      for (code ← Option(e.getCodeOrNull))
        messageCodes.addBinding(e.level, code)
    }
  }
}


private object SpoolerProcessAfterIT {
  private class MyMutableMultiMap[A, B] extends mutable.HashMap[A, mutable.Set[B]] with mutable.MultiMap[A, B]

  private case class MyFinishedEvent(orderKey: OrderKey, state: OrderState, spoolerProcessAfterParameterOption: Option[Boolean]) extends Event
}
