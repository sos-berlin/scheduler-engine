package com.sos.scheduler.engine.tests.jira.js866

import com.sos.scheduler.engine.data.job.{JobPath, TaskEnded, TaskKey, TaskStarted}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js866.JS866IT._
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS866IT extends FunSuite with ScalaSchedulerTest {

  protected override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    terminateOnError = false)

  test("kill local API job") {
    checkedKillTask(localJobPath)
  }

  //test("kill remote API job") {
  //  val h = "localhost:"+ instance[SchedulerConfiguration].tcpPort
  //  scheduler executeXml <process_class name="remote" remote_scheduler={h}/>
  //  checkedKillTask(remoteJobPath)
  //}

  private def checkedKillTask(jobPath: JobPath): Unit = {
    val eventPipe = controller.newEventPipe()
    scheduler executeXml <start_job job={jobPath.string}/>
    val taskId = eventPipe.nextWhen[TaskStarted.type] { _.key.jobPath == jobPath } .key.taskId
    scheduler executeXml <kill_task job={jobPath.string} id={taskId.number.toString} immediately="yes"/>
    eventPipe.next[TaskEnded](TaskKey(jobPath, taskId))
  }
}

private object JS866IT {
  val localJobPath = JobPath("/test")
  val remoteJobPath = JobPath("/test-remote")
}
