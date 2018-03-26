package com.sos.scheduler.engine.tests.jira.js793

import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.exceptions.TestFailedException
import org.scalatest.junit.JUnitRunner

/**
  * This test shows how to set specific parameter for jobchain nodes. To do this, you have to specify order parameters
  * with the state of the jobchain_node for which the parameter you want to set following by a slash (<l>/</l>).
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS793IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  "JS-793 shell job" in {
    runTest(JobChainPath("/test-shell"))
  }

  "JS-1764 shell job on Agent" in {
    runTest(JobChainPath("/test-shell-agent"))
  }

  "JS-1573 API job - FAILS" in {
    intercept[TestFailedException] {
      runTest(JobChainPath("/test-api"))
    }
  }

  private def runTest(jobChainPath: JobChainPath): Unit = {
    val result = runOrder(OrderCommand(
      jobChainPath orderKey "1",
      parameters = Map(
        "100/NODE_PARAM" → "TEST-VALUE-100",
        "200/NODE_PARAM" → "TEST-VALUE-200")))
    val logString = result.logString
    assert(logString contains "/SEEN_NODE_PARAM=TEST-VALUE-100/")
    assert(logString contains "/SEEN_NODE_PARAM=TEST-VALUE-200/")
  }
}
