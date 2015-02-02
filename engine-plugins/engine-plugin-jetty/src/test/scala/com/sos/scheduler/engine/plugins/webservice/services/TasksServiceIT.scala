package com.sos.scheduler.engine.plugins.webservice.services

import com.sos.scheduler.engine.data.job.{JobPath, TaskClosedEvent, TaskId, TaskStartedEvent}
import com.sos.scheduler.engine.plugins.jetty.test.JettyPluginJerseyTester
import com.sos.scheduler.engine.plugins.webservice.services.TasksServiceIT._
import com.sos.scheduler.engine.plugins.webservice.tests.Tests
import com.sos.scheduler.engine.test.configuration.{DefaultDatabaseConfiguration, TestConfiguration}
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sun.jersey.api.client.ClientResponse.Status.BAD_REQUEST
import com.sun.jersey.api.client.UniformInterfaceException
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class TasksServiceIT extends FreeSpec with ScalaSchedulerTest with JettyPluginJerseyTester {

  override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    testPackage = Some(Tests.testPackage),
    database = Some(DefaultDatabaseConfiguration()))

  "Task log" in {
    val eventPipe = controller.newEventPipe()
    controller.scheduler executeXml <start_job job={testJobPath.string}/>
    val taskId = eventPipe.nextWithCondition[TaskStartedEvent](_.jobPath == testJobPath).taskId
    val runningLog = getLog(taskId)
    runningLog should include (startMessage)
    runningLog should not include endMessage
    eventPipe.nextKeyed[TaskClosedEvent](taskId)
    getLog(taskId) should include (endMessage)   // Protokoll aus Datenbankarchiv
  }

  "Get task log of unknown TaskId should gracefully fail" in {
    intercept[UniformInterfaceException] { getLog(TaskId(999999999)) }
      .getResponse.getStatus should equal (BAD_REQUEST.getStatusCode)
  }

  private def getLog(taskId: TaskId) =
    get[String](s"/jobscheduler/engine/tasks/${taskId.string}/log", Accept = List(TEXT_PLAIN_TYPE))
}

private object TasksServiceIT {
  private val testJobPath = JobPath("/a")
  private val startMessage = "SCHEDULER-918  state=starting"
  private val endMessage = "TASK ENDS"
}
