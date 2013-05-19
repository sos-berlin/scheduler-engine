package com.sos.scheduler.engine.tests.jira.js866

import JS866IT._
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.{TaskEndedEvent, TaskStartedEvent}
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConfiguration
import com.sos.scheduler.engine.test.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.test.util.Sockets._
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS866IT extends FunSuite with ScalaSchedulerTest {

  protected override lazy val testConfiguration = TestConfiguration(
    mainArguments = List("-tcp-port="+ findAvailablePort()))

  override def checkedBeforeAll() {
    controller.setTerminateOnError(false)
    controller.activateScheduler("-tcp-port="+ findAvailablePort())
  }

  test("kill local API job") {
    checkedKillTask(localJobPath)
  }

  test("kill remote API job") {
    val h = "localhost:"+ instance[SchedulerConfiguration].tcpPort
    scheduler executeXml <process_class name="remote" remote_scheduler={h}/>
    checkedKillTask(remoteJobPath)
  }

  private def checkedKillTask(jobPath: JobPath) {
    val eventPipe = controller.newEventPipe()
    scheduler executeXml <start_job job={jobPath.string}/>
    val taskId = eventPipe.nextWithCondition[TaskStartedEvent] { _.jobPath == jobPath } .taskId
    scheduler executeXml <kill_task job={jobPath.string} id={taskId.value.toString} immediately="yes"/>
    eventPipe.nextWithCondition[TaskEndedEvent] { _.jobPath == jobPath }
  }
}

private object JS866IT {
  val localJobPath = JobPath.of("/test")
  val remoteJobPath = JobPath.of("/test-remote")
}
