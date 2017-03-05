package com.sos.scheduler.engine.tests.jira.js973

import com.sos.jobscheduler.base.generic.IsString
import com.sos.jobscheduler.common.scalautil.Closers.implicits._
import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.jobscheduler.common.scalautil.Futures._
import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.utils.FreeTcpPortFinder._
import com.sos.jobscheduler.data.event.{Event, KeyedEvent}
import com.sos.jobscheduler.data.job.TaskId
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.data.job.{JobOverview, JobPath, JobState, TaskStarted}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.log.{ErrorLogged, WarningLogged}
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.kernel.extrascheduler.ExtraScheduler
import com.sos.scheduler.engine.kernel.job.JobSubsystemClient
import com.sos.scheduler.engine.kernel.order.OrderSubsystemClient
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.remoteSchedulerParameterName
import com.sos.scheduler.engine.main.CppBinary
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.{HasCloserBeforeAndAfterAll, ScalaSchedulerTest}
import com.sos.scheduler.engine.tests.jira.js973.JS973IT._
import java.nio.file.Files
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS973IT extends FreeSpec with ScalaSchedulerTest with HasCloserBeforeAndAfterAll {

  private lazy val tcpPorts = findRandomFreeTcpPortIterator()
  private lazy val aAgent = newAgent(1).closeWithCloser
  private lazy val bAgent = newAgent(2).closeWithCloser
  private lazy val processClassAgent = newAgent(3).closeWithCloser
  private lazy val agents = List(aAgent, bAgent, processClassAgent)

  override def checkedBeforeAll(): Unit = {
    controller.startScheduler()
    agents foreach { _.extraScheduler.start() }   // Parallel mit Test-Scheduler starten
    controller.waitUntilSchedulerIsActive()
    scheduler executeXml <process_class name="test-c" remote_scheduler={processClassAgent.extraScheduler.tcpAddress.string}/>
  }

  s"Without parameter $remoteSchedulerParameterName runs job in our scheduler" in {
    testOrderWithRemoteScheduler(ShellJobChainPath, None, "**")
  }

  s"Empty parameter $remoteSchedulerParameterName runs job in our scheduler" in {
    testOrderWithRemoteScheduler(ShellJobChainPath, Some(SchedulerAddressString("")), "**")
  }

  s"Order parameter $remoteSchedulerParameterName selects a remote scheduler" in {
    testOrderWithRemoteScheduler(ShellJobChainPath, aAgent)
  }

  s"Task runs on remote_scheduler of process_class" in {
    testOrderWithRemoteScheduler(ProcessClassJobChainPath, None, processClassAgent.expectedResult)
  }

  s"Order parameter $remoteSchedulerParameterName overrides remote scheduler of process class" in {
    testOrderWithRemoteScheduler(ProcessClassJobChainPath, aAgent)
  }

  "Shell with monitor" in {
    testOrderWithRemoteScheduler(ShellWithMonitorJobChainPath, aAgent)
  }

  "An API task ignores scheduer.remote_scheduler" in {
    withEventPipe { eventPipe ⇒
      testOrderWithRemoteScheduler(ApiJobChainPath, aAgent, expectedResult = "**")
      eventPipe.nextWhen[WarningLogged](_.event.codeOption == Some(MessageCode("SCHEDULER-484")), 0.s )
      eventPipe.nextWhen[TaskStarted.type](_.key.jobPath == ApiJobPath, 0.s).key.taskId
    }
  }

//  s"For an order, the API task running on right remote scheduler is selected" in {
//    withEventPipe { eventPipe ⇒
//      testOrderWithRemoteScheduler(ApiJobChainPath, aAgent)
//      val aTaskId = eventPipe.nextWhen[TaskStarted](0.s) { _.jobPath == ApiJobPath }.taskId
//      testOrderWithRemoteScheduler(ApiJobChainPath, bAgent)
//      val bTaskId = eventPipe.nextWhen[TaskStarted](0.s) { _.jobPath == ApiJobPath }.taskId
//      testOrderWithRemoteScheduler(ApiJobChainPath, aAgent, aTaskId)
//      intercept[EventPipe.TimeoutException] { eventPipe.nextWhen[TaskStarted](0.s) { _.jobPath == ApiJobPath } }
//      testOrderWithRemoteScheduler(ApiJobChainPath, bAgent, bTaskId)
//      intercept[EventPipe.TimeoutException] { eventPipe.nextWhen[TaskStarted](0.s) { _.jobPath == ApiJobPath } }
//    }
//  }

  s"Invalid syntax of $remoteSchedulerParameterName keeps order at same job node and stops job" in {
    testInvalidJobChain(ShellJobChainPath, SchedulerAddressString(":INVALID-ADDRESS"), expectedErrorCode = MessageCode("Z-4003"))
  }

  "File_order_sink_module::create_instance_impl" in {
    val fileOrdersDir = testEnvironment.newFileOrderSourceDirectory()
    val orderFile = fileOrdersDir.toFile / "test.txt"
    orderFile.contentString = "test"
    val jobChainPath = JobChainPath("/test-file-order")
    val orderKey = jobChainPath.orderKey(orderFile.getAbsolutePath)
    eventBus.awaiting[OrderFinished](orderKey) {
      scheduler executeXml
        <job_chain name={jobChainPath.name}>
          <file_order_source directory={fileOrdersDir.toString} regex="^test\.txt$"/>
          <job_chain_node state="100" job="test-a"/>
          <file_order_sink state="end" remove="yes"/>
        </job_chain>
    }
    Files.delete(fileOrdersDir)
  }

  //test("Not in a cluster") {} ⇒ SCHEDULER-483

//  JS-973 gilt nur für Aufträge, nicht für Task-Starts
//  ignore(s"Order parameter overrides task parameter") {
//    // Oder sollte ein leerer AUftragsparameter den Task-Parameter gelten lassen? "" ist wie nicht angegeben.
//    pending
//  }

//  JS-973 gilt nur für Aufträge, nicht für Task-Starts
//  ignore(s"Empty order parameter overrides task parameter") {
//    pending
//  }

  private def newAgent(id: Int): Agent = {
    val tcpPort = tcpPorts.next()
    val args = List(
      controller.cppBinaries.file(CppBinary.exeFilename).getPath,
      s"-sos.ini=${controller.environment.sosIniFile}",
      s"-ini=${controller.environment.iniFile}",
      s"-id=agent-$id-$tcpPort",
      s"-roles=agent",
      s"-log-dir=${controller.environment.logDirectory.getPath}",
      s"-log-level=debug9",
      s"-log=+${controller.environment.schedulerLog.getPath}",
      s"-java-classpath=${sys.props("java.class.path")}",
      s"-job-java-classpath=${sys.props("java.class.path")}",
      s"-e",
      s"-config=" + (controller.environment.configDirectory / "agent-scheduler.xml"),
      s"-configuration-directory=${controller.environment.liveDirectory}")
    val testValue = s"TEST-$tcpPort"
    new Agent(new ExtraScheduler(args, List(TestVariableName -> testValue), tcpPort=Some(tcpPort)), s"*$testValue*")
  }

  private def testOrderWithRemoteScheduler(jobChainPath: JobChainPath, agent: Agent): Unit = {
    testOrderWithRemoteScheduler(jobChainPath, agent, agent.expectedResult)
  }

  private def testOrderWithRemoteScheduler(jobChainPath: JobChainPath, agent: Agent, taskId: TaskId): Unit = {
    testOrderWithRemoteScheduler(jobChainPath, agent, s"${agent.expectedResult} taskId=$taskId")
  }

  private def testOrderWithRemoteScheduler(jobChainPath: JobChainPath, agent: Agent, expectedResult: String): Unit = {
    testOrderWithRemoteScheduler(jobChainPath, Some(SchedulerAddressString(agent.extraScheduler.tcpAddress.string)), expectedResult)
  }

  private def testOrderWithRemoteScheduler(jobChainPath: JobChainPath, remoteScheduler: Option[SchedulerAddressString], expectedResult: String): Unit = {
    testOrderWithRemoteScheduler(newOrderKey(jobChainPath), remoteScheduler, expectedResult)
  }

  private def testOrderWithRemoteScheduler(orderKey: OrderKey, remoteScheduler: Option[SchedulerAddressString], expectedResult: String): Unit = {
    withEventPipe { eventPipe ⇒
      scheduler executeXml newOrder(orderKey, remoteScheduler)
      eventPipe.nextWhen[OrderFinishedWithResultEvent] { _.key == orderKey }.event.result should startWith(expectedResult)
    }
  }

  private def testInvalidJobChain(jobChainPath: JobChainPath, remoteScheduler: SchedulerAddressString, expectedErrorCode: MessageCode): Unit = {
    val eventPipe = controller.newEventPipe()
    val orderKey = newOrderKey(jobChainPath)
    controller.suppressingTerminateOnError {
      val firstJobPath = instance[OrderSubsystemClient].jobChain(jobChainPath).jobNodes.head.jobPath
      instance[JobSubsystemClient].jobView[JobOverview](firstJobPath).state shouldEqual JobState.pending
      scheduler executeXml newOrder(orderKey, Some(remoteScheduler))
      eventPipe.nextAny[ErrorLogged].event.codeOption shouldEqual Some(expectedErrorCode)
      eventPipe.next[OrderStepEnded](orderKey).nodeTransition shouldEqual OrderNodeTransition.Keep
      instance[JobSubsystemClient].jobView[JobOverview](firstJobPath).state shouldEqual JobState.stopped
    }
  }

  eventBus.onHot[OrderFinished] {
    case KeyedEvent(orderKey, _) ⇒
      eventBus.publishCold(KeyedEvent(OrderFinishedWithResultEvent(orderDetailed(orderKey).variables.getOrElse(ResultVariableName, "")))(orderKey))
  }

  private val orderIdGenerator = (1 to Int.MaxValue).iterator map { i => new OrderId(i.toString) }

  def newOrderKey(o: JobChainPath) =
    o.orderKey(orderIdGenerator.next())
}

private object JS973IT {
  private val ShellJobChainPath = JobChainPath("/test-shell")
  private val ShellWithMonitorJobChainPath = JobChainPath("/test-shell-with-monitor")
  private val ApiJobPath = JobPath("/test-api")
  private val ApiJobChainPath = JobChainPath("/test-api")
  private val ProcessClassJobChainPath = JobChainPath("/test-processClass")
  private val TestVariableName = "TEST_WHICH_SCHEDULER"
  private val ResultVariableName = "result"
  private val ExtraSchedulerTimeout = 60.s
  private val logger = Logger(getClass)

  private def newOrder(orderKey: OrderKey, remoteSchedulerOption: Option[SchedulerAddressString]) =
    <order job_chain={orderKey.jobChainPath.string} id={orderKey.id.string}>
      <params>{
        remoteSchedulerOption.toList map { o => <param name={remoteSchedulerParameterName} value={o.string}/>}
      }</params>
    </order>

  private case class OrderFinishedWithResultEvent(result: String) extends Event {
    type Key = OrderKey
  }

  private class Agent(val extraScheduler: ExtraScheduler, _expectedResult: String) extends AutoCloseable {
    def close(): Unit = {
      logger.info(s"close start $extraScheduler")
      extraScheduler.close()
      logger.info(s"close finished $extraScheduler")
    }

    def expectedResult = {
      awaitResult(extraScheduler.activatedFuture, ExtraSchedulerTimeout)
      _expectedResult
    }
  }

  private case class SchedulerAddressString(string: String) extends IsString
}
