package com.sos.scheduler.engine.tests.jira.js973

import JS973IT._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder._
import com.sos.scheduler.engine.common.utils.SosAutoCloseable
import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.log.ErrorLogEvent
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.eventbus.HotEventHandler
import com.sos.scheduler.engine.kernel.job.{JobState, JobSubsystem}
import com.sos.scheduler.engine.kernel.order.{OrderSubsystem, UnmodifiableOrder}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.remoteSchedulerParameterName
import com.sos.scheduler.engine.main.CppBinary
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.tests.jira.js973.JS973IT.OrderFinishedWithResultEvent
import java.io.File
import org.scalatest.matchers.ShouldMatchers._
import scala.concurrent.Await

final class JS973IT extends ScalaSchedulerTest {

  private lazy val aSlave = newSlave()
  private lazy val bSlave = newSlave()
  private lazy val processClassSlave = newSlave()
  private lazy val slaves = List(aSlave, bSlave, processClassSlave)

  override def checkedBeforeAll() {
    controller.startScheduler()
    slaves foreach { _.extraScheduler.start() }   // Parallel mit Test-Scheduler starten
    controller.waitUntilSchedulerIsActive()
    scheduler executeXml <process_class name="test-c" remote_scheduler={processClassSlave.extraScheduler.address.string}/>
  }

  override def afterAll() {
    super.afterAll()
    slaves foreach { _.close() }
  }

  test(s"Without parameter $remoteSchedulerParameterName runs job in our scheduler") {
    testOrderWithRemoteScheduler(shellJobChainPath, None, "**")
  }

  test(s"Empty parameter $remoteSchedulerParameterName runs job in our scheduler") {
    testOrderWithRemoteScheduler(shellJobChainPath, Some(SchedulerAddress("")), "**")
  }

  test(s"Order parameter $remoteSchedulerParameterName selects a remote scheduler") {
    testOrderWithRemoteScheduler(shellJobChainPath, aSlave)
  }

  test(s"Task runs on remote_scheduler of process_class") {
    testOrderWithRemoteScheduler(processClassJobChainPath, None, processClassSlave.expectedResult)
  }

  test(s"Order parameter $remoteSchedulerParameterName overrides remote scheduler of process class") {
    testOrderWithRemoteScheduler(processClassJobChainPath, aSlave)
  }

  test("Shell with monitor") {
    testOrderWithRemoteScheduler(shellWithMonitorJobChainPath, aSlave)
  }

//  JS-973 gilt nicht für API-Tasks.
//  test(s"For an order, the task running on right remote scheduler is selected") {
//    val eventPipe = controller.newEventPipe()
//    testOrderWithRemoteScheduler(apiJobChainPath, aSlave)
//    val aTaskId = eventPipe.nextWithTimeoutAndCondition[TaskStartedEvent](0.s) { _.jobPath == apiJobPath } .taskId
//    testOrderWithRemoteScheduler(apiJobChainPath, bSlave)
//    val bTaskId = eventPipe.nextWithTimeoutAndCondition[TaskStartedEvent](0.s) { _.jobPath == apiJobPath } .taskId
//    testOrderWithRemoteScheduler(apiJobChainPath, aSlave, aTaskId)
//    intercept[EventPipe.TimeoutException] { eventPipe.nextWithTimeoutAndCondition[TaskStartedEvent](0.s) { _.jobPath == apiJobPath } }
//    testOrderWithRemoteScheduler(apiJobChainPath, bSlave, bTaskId)
//    intercept[EventPipe.TimeoutException] { eventPipe.nextWithTimeoutAndCondition[TaskStartedEvent](0.s) { _.jobPath == apiJobPath } }
//    eventPipe.close()
//  }

  test(s"Invalid syntax of $remoteSchedulerParameterName keeps order at same job node and stops job") {
    testInvalidJobChain(shellJobChainPath, SchedulerAddress(":INVALID-ADDRESS"), expectedErrorCode = "Z-4003")
  }

  test("Not for API jobs") {
    testInvalidJobChain(apiJobChainPath, aSlave.extraScheduler.address, expectedErrorCode = "SCHEDULER-484")
  }

  //test("Not in a cluster") {}
  //TODO Nicht für Cluster-Betrieb. In spooler_task prüfen.

//  JS-974 gilt nur für Aufträge, nicht für Task-Starts
//  ignore(s"Order parameter overrides task parameter") {
//    // Oder sollte ein leerer AUftragsparameter den Task-Parameter gelten lassen? "" ist wie nicht angegeben.
//    pending
//  }

//  JS-974 gilt nur für Aufträge, nicht für Task-Starts
//  ignore(s"Empty order parameter overrides task parameter") {
//    pending
//  }

  private def newSlave(): Slave = {
    val tcpPort = findRandomFreePort(20000 until 30000)
    val args = List(
      controller.cppBinaries.file(CppBinary.exeFilename).getPath,
      s"-sos.ini=${controller.environment.sosIniFile}",
      s"-ini=${controller.environment.iniFile}",
      s"-id=slave-$tcpPort",
      s"-log-dir=${controller.environment.logDirectory.getPath}",
      s"-log-level=debug9",
      s"-log=+${controller.environment.schedulerLog.getPath}",
      s"-java-classpath=${System.getProperty("java.class.path")}",
      s"-job-java-classpath=${System.getProperty("java.class.path")}",
      s"-e",
      new File(controller.environment.configDirectory.getPath, "slave-scheduler.xml").getPath)
    val testValue = s"TEST-$tcpPort"
    new Slave(new ExtraScheduler(args, List(testVariableName -> testValue), tcpPort), s"*$testValue*")
  }

  private def testOrderWithRemoteScheduler(jobChainPath: JobChainPath, slave: Slave) {
    testOrderWithRemoteScheduler(jobChainPath, Some(slave.extraScheduler.address), slave.expectedResult)
  }

  private def testOrderWithRemoteScheduler(jobChainPath: JobChainPath, remoteScheduler: Option[SchedulerAddress], expectedResult: String) {
    testOrderWithRemoteScheduler(newOrderKey(jobChainPath), remoteScheduler, expectedResult)
  }

  private def testOrderWithRemoteScheduler(orderKey: OrderKey, remoteScheduler: Option[SchedulerAddress], expectedResult: String) {
    val eventPipe = controller.newEventPipe()
    scheduler executeXml newOrder(orderKey, remoteScheduler)
    eventPipe.nextWithCondition[OrderFinishedWithResultEvent] { _.orderKey == orderKey } .result should startWith (expectedResult)
  }

  private def testInvalidJobChain(jobChainPath: JobChainPath, remoteScheduler: SchedulerAddress, expectedErrorCode: String) {
    val eventPipe = controller.newEventPipe()
    val orderKey = newOrderKey(jobChainPath)
    controller.suppressingTerminateOnError {
      val firstJobPath = instance[OrderSubsystem].jobChain(jobChainPath).jobNodes.head.jobPath
      instance[JobSubsystem].job(firstJobPath).state should equal (JobState.pending)
      scheduler executeXml newOrder(orderKey, Some(remoteScheduler))
      eventPipe.next[ErrorLogEvent].getCodeOrNull should equal (expectedErrorCode)
      eventPipe.nextWithCondition[OrderStepEndedEvent] { _.orderKey == orderKey } .stateTransition should equal (OrderStateTransition.keepState)
      instance[JobSubsystem].job(firstJobPath).state should equal (JobState.stopped)
    }
  }

  @HotEventHandler
  def handle(e: OrderFinishedEvent, o: UnmodifiableOrder) {
    controller.getEventBus.publishCold(OrderFinishedWithResultEvent(e.orderKey, o.getParameters(resultVariableName)))
  }

  private val orderIdGenerator = (1 to Int.MaxValue).iterator map { i => new OrderId(i.toString) }

  def newOrderKey(o: JobChainPath) =
    o.orderKey(orderIdGenerator.next())
}

private object JS973IT {
  private val shellJobChainPath = JobChainPath.of("/test-shell")
  private val shellWithMonitorJobChainPath = JobChainPath.of("/test-shell-with-monitor")
  private val apiJobChainPath = JobChainPath.of("/test-api")
  private val processClassJobChainPath = JobChainPath.of("/test-processClass")
  private val testVariableName = "TEST_WHICH_SCHEDULER"
  private val resultVariableName = "result"
  private val extraSchedulerTimeout = 60.s
  private val logger = Logger(getClass)

  private def newOrder(orderKey: OrderKey, remoteSchedulerOption: Option[SchedulerAddress]) =
    <order job_chain={orderKey.jobChainPathString} id={orderKey.getId}>
      <params>{
        remoteSchedulerOption.toList map { o => <param name={remoteSchedulerParameterName} value={o.string}/>}
      }</params>
    </order>

  private case class OrderFinishedWithResultEvent(orderKey: OrderKey, result: String) extends OrderEvent

  private class Slave(val extraScheduler: ExtraScheduler, _expectedResult: String) extends SosAutoCloseable {
    def close() {
      logger info s"close start $extraScheduler"
      extraScheduler.close()
      logger info s"close finished $extraScheduler"
    }

    def expectedResult = {
      Await.result(extraScheduler.tcpIsReadyFuture, extraSchedulerTimeout.toScalaDuration)
      _expectedResult
    }
  }
}
