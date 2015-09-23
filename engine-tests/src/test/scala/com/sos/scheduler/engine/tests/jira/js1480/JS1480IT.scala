package com.sos.scheduler.engine.tests.jira.js1480

import com.sos.scheduler.engine.agent.client.AgentClientFactory
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.data.job.{JobPath, TaskId}
import com.sos.scheduler.engine.data.log.InfoLogEvent
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1480.JS1480IT._
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
    eventBus.awaitingEvent[InfoLogEvent](_.message contains TestJob.LogLine) {
      runJobFuture(TestJobPath)
    }
    val agentClient = instance[AgentClientFactory].apply(agentUri)
    val view = awaitSuccess(agentClient.task.overview)
    logger.info(view.toString)
    assert(!view.isTerminating)
    assert(view.currentTaskCount == 1)
    assert(view.totalTaskCount == 1)
    assert(view.tasks.size == view.currentTaskCount)
    val task = view.tasks(0)
    assert(task.arguments.get.taskId == TaskId(SchedulerConstants.taskIdOffset))
    assert(task.arguments.get.jobName == TestJobPath.name)
    assert(task.arguments.get.language == "java")
    assert(task.arguments.get.javaClassName contains classOf[TestJob].getName)
    assert(task.arguments.get.monitorCount == 0)
  }
}

private object JS1480IT {
  private val TestJobPath = JobPath("/test")
  private val logger = Logger(getClass)
}
