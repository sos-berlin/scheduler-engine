package com.sos.scheduler.engine.tests.jira.js973

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder._
import com.sos.scheduler.engine.data.base.IsString
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.log.ErrorLogEvent
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.eventbus.HotEventHandler
import com.sos.scheduler.engine.kernel.extrascheduler.ExtraScheduler
import com.sos.scheduler.engine.kernel.job.{JobState, JobSubsystem}
import com.sos.scheduler.engine.kernel.order.{OrderSubsystem, UnmodifiableOrder}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.remoteSchedulerParameterName
import com.sos.scheduler.engine.main.CppBinary
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.test.scalatest.HasCloserBeforeAndAfterAll
import com.sos.scheduler.engine.tests.jira.js973.JS973IT._
import java.io.File
import java.nio.file.Files
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.concurrent.Await

@RunWith(classOf[JUnitRunner])
final class JS973IT extends FreeSpec with ScalaSchedulerTest with HasCloserBeforeAndAfterAll {

  private lazy val aAgent = newAgent(1)
  private lazy val bAgent = newAgent(2)
  private lazy val processClassAgent = newAgent(3)
  private lazy val agents = List(aAgent, bAgent, processClassAgent)

  override def checkedBeforeAll() {
    controller.startScheduler()
    agents foreach { _.extraScheduler.start() }   // Parallel mit Test-Scheduler starten
    controller.waitUntilSchedulerIsActive()
    scheduler executeXml <process_class name="test-c" remote_scheduler={processClassAgent.extraScheduler.address.string}/>
  }

  override def afterAll() {
    super.afterAll()
    agents foreach { _.close() }
  }

  s"Without parameter $remoteSchedulerParameterName runs job in our scheduler" in {
    testOrderWithRemoteScheduler(shellJobChainPath, None, "**")
  }

  s"Empty parameter $remoteSchedulerParameterName runs job in our scheduler" in {
    testOrderWithRemoteScheduler(shellJobChainPath, Some(SchedulerAddressString("")), "**")
  }

  s"Order parameter $remoteSchedulerParameterName selects a remote scheduler" in {
    testOrderWithRemoteScheduler(shellJobChainPath, aAgent)
  }

  s"Task runs on remote_scheduler of process_class" in {
    testOrderWithRemoteScheduler(processClassJobChainPath, None, processClassAgent.expectedResult)
  }

  s"Order parameter $remoteSchedulerParameterName overrides remote scheduler of process class" in {
    testOrderWithRemoteScheduler(processClassJobChainPath, aAgent)
  }

  "Shell with monitor" in {
    testOrderWithRemoteScheduler(shellWithMonitorJobChainPath, aAgent)
  }

//  JS-973 gilt nicht für API-Tasks.
//  test(s"For an order, the task running on right remote scheduler is selected") {
//    val eventPipe = controller.newEventPipe()
//    testOrderWithRemoteScheduler(apiJobChainPath, aAgent)
//    val aTaskId = eventPipe.nextWithTimeoutAndCondition[TaskStartedEvent](0.s) { _.jobPath == apiJobPath } .taskId
//    testOrderWithRemoteScheduler(apiJobChainPath, bAgent)
//    val bTaskId = eventPipe.nextWithTimeoutAndCondition[TaskStartedEvent](0.s) { _.jobPath == apiJobPath } .taskId
//    testOrderWithRemoteScheduler(apiJobChainPath, aAgent, aTaskId)
//    intercept[EventPipe.TimeoutException] { eventPipe.nextWithTimeoutAndCondition[TaskStartedEvent](0.s) { _.jobPath == apiJobPath } }
//    testOrderWithRemoteScheduler(apiJobChainPath, bAgent, bTaskId)
//    intercept[EventPipe.TimeoutException] { eventPipe.nextWithTimeoutAndCondition[TaskStartedEvent](0.s) { _.jobPath == apiJobPath } }
//    eventPipe.close()
//  }

  s"Invalid syntax of $remoteSchedulerParameterName keeps order at same job node and stops job" in {
    testInvalidJobChain(shellJobChainPath, SchedulerAddressString(":INVALID-ADDRESS"), expectedErrorCode = MessageCode("Z-4003"))
  }

  "Not for API jobs" in {
    testInvalidJobChain(apiJobChainPath, SchedulerAddressString(aAgent.extraScheduler.address.string), expectedErrorCode = MessageCode("SCHEDULER-484"))
  }

  "File_order_sink_module::create_instance_impl" in {
    val fileOrdersDir = Files.createTempDirectory("test")
    val orderFile = fileOrdersDir.toFile / "test.txt"
    orderFile.write("test")
    val jobChainPath = JobChainPath("/test-file-order")
    val orderKey = jobChainPath.orderKey(orderFile.getAbsolutePath)
    controller.getEventBus.awaitingKeyedEvent[OrderFinishedEvent](orderKey) {
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
    val tcpPort = findRandomFreeTcpPort()
    val args = List(
      controller.cppBinaries.file(CppBinary.exeFilename).getPath,
      s"-sos.ini=${controller.environment.sosIniFile}",
      s"-ini=${controller.environment.iniFile}",
      s"-id=agent-$id-$tcpPort",
      s"-roles=agent",
      s"-log-dir=${controller.environment.logDirectory.getPath}",
      s"-log-level=debug9",
      s"-log=+${controller.environment.schedulerLog.getPath}",
      s"-java-classpath=${System.getProperty("java.class.path")}",
      s"-job-java-classpath=${System.getProperty("java.class.path")}",
      s"-e",
      new File(controller.environment.configDirectory.getPath, "agent-scheduler.xml").getPath)
    val testValue = s"TEST-$tcpPort"
    new Agent(new ExtraScheduler(args, List(testVariableName -> testValue), tcpPort), s"*$testValue*")
  }

  private def testOrderWithRemoteScheduler(jobChainPath: JobChainPath, agent: Agent) {
    testOrderWithRemoteScheduler(jobChainPath, Some(SchedulerAddressString(agent.extraScheduler.address.string)), agent.expectedResult)
  }

  private def testOrderWithRemoteScheduler(jobChainPath: JobChainPath, remoteScheduler: Option[SchedulerAddressString], expectedResult: String) {
    testOrderWithRemoteScheduler(newOrderKey(jobChainPath), remoteScheduler, expectedResult)
  }

  private def testOrderWithRemoteScheduler(orderKey: OrderKey, remoteScheduler: Option[SchedulerAddressString], expectedResult: String) {
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      scheduler executeXml newOrder(orderKey, remoteScheduler)
      eventPipe.nextWithCondition[OrderFinishedWithResultEvent] {_.orderKey == orderKey}.result should startWith(expectedResult)
    }
  }

  private def testInvalidJobChain(jobChainPath: JobChainPath, remoteScheduler: SchedulerAddressString, expectedErrorCode: MessageCode) {
    val eventPipe = controller.newEventPipe()
    val orderKey = newOrderKey(jobChainPath)
    controller.suppressingTerminateOnError {
      val firstJobPath = instance[OrderSubsystem].jobChain(jobChainPath).jobNodes.head.jobPath
      instance[JobSubsystem].job(firstJobPath).state shouldEqual JobState.pending
      scheduler executeXml newOrder(orderKey, Some(remoteScheduler))
      eventPipe.nextAny[ErrorLogEvent].codeOption shouldEqual Some(expectedErrorCode)
      eventPipe.nextWithCondition[OrderStepEndedEvent] { _.orderKey == orderKey } .stateTransition shouldEqual OrderStateTransition.keepState
      instance[JobSubsystem].job(firstJobPath).state shouldEqual JobState.stopped
    }
  }

  @HotEventHandler
  def handle(e: OrderFinishedEvent, o: UnmodifiableOrder) {
    controller.getEventBus.publishCold(OrderFinishedWithResultEvent(e.orderKey, o.parameters(resultVariableName)))
  }

  private val orderIdGenerator = (1 to Int.MaxValue).iterator map { i => new OrderId(i.toString) }

  def newOrderKey(o: JobChainPath) =
    o.orderKey(orderIdGenerator.next())
}

private object JS973IT {
  private val shellJobChainPath = JobChainPath("/test-shell")
  private val shellWithMonitorJobChainPath = JobChainPath("/test-shell-with-monitor")
  private val apiJobChainPath = JobChainPath("/test-api")
  private val processClassJobChainPath = JobChainPath("/test-processClass")
  private val testVariableName = "TEST_WHICH_SCHEDULER"
  private val resultVariableName = "result"
  private val extraSchedulerTimeout = 60.s
  private val logger = Logger(getClass)

  private def newOrder(orderKey: OrderKey, remoteSchedulerOption: Option[SchedulerAddressString]) =
    <order job_chain={orderKey.jobChainPath.string} id={orderKey.id.string}>
      <params>{
        remoteSchedulerOption.toList map { o => <param name={remoteSchedulerParameterName} value={o.string}/>}
      }</params>
    </order>

  private case class OrderFinishedWithResultEvent(orderKey: OrderKey, result: String) extends OrderEvent

  private class Agent(val extraScheduler: ExtraScheduler, _expectedResult: String) extends AutoCloseable {
    def close() {
      logger.info(s"close start $extraScheduler")
      extraScheduler.close()
      logger.info(s"close finished $extraScheduler")
    }

    def expectedResult = {
      Await.result(extraScheduler.isActiveFuture, extraSchedulerTimeout.toScalaDuration)
      _expectedResult
    }
  }

  private case class SchedulerAddressString(string: String) extends IsString
}
