package com.sos.scheduler.engine.tests.jira.js1655

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits.RichFile
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1655IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  "agent_url" in {
    val taskResult = runJob(JobPath("/test"))
    assert(taskResult.logString contains s"AGENT_URL=${agent.localUri}")
  }

  "Command rotate_logs" in {
    def isKnownLog = testEnvironment.schedulerLog.contentString contains "INSERT into SCHEDULER_TASKS"
    assert(isKnownLog)
    scheduler executeXml <modify_spooler cmd="rotate_logs"/>
    assert(!isKnownLog)
  }
}
