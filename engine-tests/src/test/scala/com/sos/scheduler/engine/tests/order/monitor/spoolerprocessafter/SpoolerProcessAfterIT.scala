package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter

import com.sos.jobscheduler.agent.Agent
import com.sos.jobscheduler.agent.configuration.AgentConfiguration
import com.sos.jobscheduler.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.jobscheduler.common.scalautil.Futures._
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.time.TimeoutWithSteps
import com.sos.jobscheduler.common.time.WaitForCondition.waitForCondition
import com.sos.jobscheduler.common.utils.FreeTcpPortFinder
import com.sos.jobscheduler.data.event.{Event, KeyedEvent}
import com.sos.jobscheduler.data.job.TaskId
import com.sos.jobscheduler.data.log.SchedulerLogLevel
import com.sos.scheduler.engine.data.job.{JobOverview, TaskClosed, TaskKey}
import com.sos.scheduler.engine.data.jobchain.NodeId
import com.sos.scheduler.engine.data.log.Logged
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.kernel.job.JobSubsystemClient
import com.sos.scheduler.engine.kernel.order.OrderSubsystemClient
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.SpoolerProcessAfterIT._
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.expected._
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.setting._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable

@RunWith(classOf[JUnitRunner])
final class SpoolerProcessAfterIT extends FreeSpec with ScalaSchedulerTest {

  private lazy val List(tcpPort, javaPort, agentHttpPort) = FreeTcpPortFinder.findRandomFreeTcpPorts(3)

  protected override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    mainArguments = List(
      s"-tcp-port=$tcpPort",
      s"-http-port=$javaPort"),
    terminateOnError = false)

  private lazy val agent = new Agent(AgentConfiguration.forTest(httpPort = agentHttpPort)).closeWithCloser
  private val messageCodes = new MyMutableMultiMap[SchedulerLogLevel, String]
  private lazy val jobSubsystem = instance[JobSubsystemClient]
  private lazy val orderSubsystem = instance[OrderSubsystemClient]

  private sealed abstract class AgentMode(override val toString: String, val addressOption: () ⇒ Option[String])
  private object NoAgent          extends AgentMode("No Agent"         , () ⇒ None)
  private object JavaAgent        extends AgentMode("Universal Agent"  , () ⇒ Some(s"http://127.0.0.1:$agentHttpPort"))
  private object TcpCppAgent      extends AgentMode("C++ Agent via TCP", () ⇒ Some(s"127.0.0.1:$tcpPort"))
  private val allAgentModes = List(NoAgent, JavaAgent, TcpCppAgent)

  private val expectedTaskId = Iterator from 3 map TaskId.apply
  Settings.list.zipWithIndex foreach { case ((setting, expected), i) ⇒
    val index = i + 1
    val testName = s"$index. $setting should result in $expected"
    testName - {
      for (mode ← allAgentModes) {
        s"$mode" in {
          myTest(index, mode, setting, expected, expectedTaskId.next())
        }
      }
    }
  }

  protected override def onSchedulerActivated() = awaitResult(agent.start(), 10.s)

  private def myTest(index: Int, agentMode: AgentMode, setting: Setting, expected: Expected, expectedTaskId: TaskId): Unit =
    withEventPipe { eventPipe ⇒
      deleteAndWriteConfigurationFile(ProcessClassPath("/test"), <process_class remote_scheduler={agentMode.addressOption().orNull}/>)
      val job = jobSubsystem.job(setting.jobPath)
      def jobState = jobSubsystem.jobView[JobOverview](setting.jobPath).state

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
        eventPipe.nextAny[TaskClosed.type] match {
          case KeyedEvent(TaskKey(_, taskId), _) ⇒
            assert(taskId == expectedTaskId, "TaskClosed not for expected task - probably a previous test failed")
        }
        waitForCondition(TimeoutWithSteps(3.s, 10.ms)) { jobState == expected.jobState }   // Der Job-Zustand wird asynchron geändert (stopping -> stopped, running -> due). Wir warten kurz darauf.
      }

      def checkAssertions(event: KeyedEvent[MyFinishedEvent]): Unit = {
        assert(event.key == setting.orderKey)
        assert(expected.orderStateExpectation matches event.event.nodeId, s", expected NodeId=${expected.orderStateExpectation}, but was ${event.event.nodeId}")
        assert(event.event.spoolerProcessAfterParameterOption == expected.spoolerProcessAfterParameterOption, "Parameter for spooler_process_after(): ")
        assert(jobState == expected.jobState, ", Job.state is not as expected")
        expected.requireMandatoryMessageCodes(messageCodes)
        //??? assert(expected.removeIgnorables(messageCodes).toMap  == expected.messageCodes.toMap)
      }

      def cleanUpAfterTest(): Unit = {
        scheduler executeXml <modify_job job={setting.jobPath.string} cmd="unstop"/>
        messageCodes.clear()
      }
    }

  eventBus.onHot[OrderStepEnded] {
    case KeyedEvent(orderKey, e) if e.nodeTransition == OrderNodeTransition.Keep ⇒
      // Es wird kein OrderFinished geben.
      publishMyFinishedEvent(orderDetailed(orderKey))
  }

  eventBus.onHot[OrderFinished] {
    case KeyedEvent(orderKey, _) ⇒
      publishMyFinishedEvent(orderDetailed(orderKey))
  }

  private def publishMyFinishedEvent(order: OrderDetailed): Unit = {
    eventBus.publishCold(KeyedEvent(
      MyFinishedEvent(
        order.overview.nodeId,
        order.variables.get(SpoolerProcessAfterNames.parameter) map { _.toBoolean }))
      (order.orderKey))
  }

  eventBus.onHot[Logged] { case KeyedEvent(_, e: Logged) ⇒
    if (Expected.LogLevels contains e.level) {
      for (code ← e.codeOption)
        messageCodes.addBinding(e.level, code.string)
    }
  }
}


private object SpoolerProcessAfterIT {
  private class MyMutableMultiMap[A, B] extends mutable.HashMap[A, mutable.Set[B]] with mutable.MultiMap[A, B]

  private case class MyFinishedEvent(nodeId: NodeId, spoolerProcessAfterParameterOption: Option[Boolean]) extends Event {
    type Key = OrderKey
  }
}
