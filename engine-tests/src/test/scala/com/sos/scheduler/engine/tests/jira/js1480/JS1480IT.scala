package com.sos.scheduler.engine.tests.jira.js1480

import com.sos.scheduler.engine.client.agent.SchedulerAgentClientFactory
import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.common.system.OperatingSystem.isUnix
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.log.InfoLogged
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1480.JS1480IT._
import java.net.InetAddress
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1480 Agent web services.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1480IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  "Agent web services" in {
    eventBus.awaitingWhen[InfoLogged](_.event.message contains TestJob.LogLine) {
      startJob(TestJobPath)
    }
    val agentClient = instance[SchedulerAgentClientFactory].apply(agentUri)
    val view = awaitSuccess(agentClient.task.overview)
    logger.info(view.toString)
    assert(view.currentTaskCount == 1)
    assert(view.totalTaskCount == 1)
    val tasks = awaitSuccess(agentClient.task.tasks)
    assert(tasks.size == view.currentTaskCount)
    val task = tasks(0)
    assert(task.pid.isDefined == isUnix)   // Not official, depends on JVM
    assert(task.arguments.get.language == "java")
    assert(task.arguments.get.javaClassName contains classOf[TestJob].getName)
    assert(task.arguments.get.monitorCount == 0)
    assert(task.startedByHttpIp contains InetAddress.getByName("127.0.0.1"))
  }
}

private object JS1480IT {
  private val TestJobPath = JobPath("/test")
  private val logger = Logger(getClass)
}
