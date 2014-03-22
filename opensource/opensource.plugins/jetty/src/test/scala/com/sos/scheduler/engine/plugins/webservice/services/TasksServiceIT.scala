package com.sos.scheduler.engine.plugins.webservice.services

import TasksServiceIT._
import com.sos.scheduler.engine.data.job.{JobPath, TaskClosedEvent, TaskStartedEvent}
import com.sos.scheduler.engine.plugins.jetty.tests.commons.JettyPluginTests._
import com.sos.scheduler.engine.plugins.webservice.tests.Tests
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.configuration.{DefaultDatabaseConfiguration, TestConfiguration}
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sun.jersey.api.client.UniformInterfaceException
import com.sun.jersey.api.client.ClientResponse.Status.BAD_REQUEST
import javax.ws.rs.core.MediaType._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class TasksServiceIT extends FreeSpec with ScalaSchedulerTest {

  override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    testPackage = Some(Tests.testPackage),
    database = Some(DefaultDatabaseConfiguration()))
  private lazy val tasksResource = javaResource(injector).path("tasks")

  "Task log" in {
    val taskId = controller.getEventBus.awaitingEvent[TaskStartedEvent](predicate = _.jobPath == testJobPath) {
      controller.scheduler executeXml <start_job job={testJobPath.string}/>
    }.taskId
    val runningLog = tasksResource.path(taskId.string).path("log").accept(TEXT_PLAIN).get(classOf[String])
    runningLog should include (startMessage)
    runningLog should not include endMessage
    controller.getEventBus.awaitingEvent[TaskClosedEvent](predicate = _.jobPath == testJobPath) {}
    tasksResource.path(taskId.string).path("log").accept(TEXT_PLAIN).get(classOf[String]) should include (endMessage)   // Protokoll aus Datenbankarchiv
  }

  "Get task log of unknown TaskId should gracefully fail" in {
    intercept[UniformInterfaceException] { tasksResource.path("999999999/log").accept(TEXT_PLAIN).get(classOf[String]) }
    .getResponse.getClientResponseStatus should equal (BAD_REQUEST)
  }
}

private object TasksServiceIT {
  private val testJobPath = JobPath("/a")
  private val startMessage = "SCHEDULER-918  state=starting"
  private val endMessage = "TASK ENDS"
}
