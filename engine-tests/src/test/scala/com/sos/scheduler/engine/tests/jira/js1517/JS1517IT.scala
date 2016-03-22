package com.sos.scheduler.engine.tests.jira.js1517

import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.ThreeTypesOfAgentProcessClasses
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1517IT extends FreeSpec with ScalaSchedulerTest with ThreeTypesOfAgentProcessClasses {

  addTestsForThreeTypesOfAgent(ProcessClassPath("/test")) { _ ⇒
    val result = runJob(JobPath("/test"), Map("a" → "test"))
    assert(result.logString contains "TEST_VARIABLE_A=test")
  }
}
