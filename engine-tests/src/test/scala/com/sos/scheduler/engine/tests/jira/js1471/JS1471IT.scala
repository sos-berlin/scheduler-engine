package com.sos.scheduler.engine.tests.jira.js1471

import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.OrderState
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.kernel.job.JobState
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.test.util.time.WaitForCondition.waitForCondition
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1471 JS-1471 FIXED: &lt;job_chain_node on_error="suspend"> does not work when a order job without monitor exits with error.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1471IT extends FreeSpec with ScalaSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(getClass,
    ignoreError = Set(MessageCode("SCHEDULER-280")))

  for (name ← List("test-simple", "test-monitor")) s"$name" in {
    val jobChainPath = JobChainPath(s"/$name")
    val orderKey = jobChainPath orderKey "1"
    val jobPath = JobPath(s"/$name")
    scheduler executeXml OrderCommand(orderKey)
    waitForCondition(TestTimeout, 100.ms) { job(jobPath).state == JobState.stopped }
    assert(order(orderKey).state == OrderState("100"))
    assert(order(orderKey).isSuspended)
  }
}
