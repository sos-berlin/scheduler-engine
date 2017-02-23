package com.sos.scheduler.engine.tests.jira.js1329

import com.sos.jobscheduler.data.job.ReturnCode
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.log.{ErrorLogged, InfoLogged}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.ProcessClassConfiguration
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
  * JS-1329, JS-1615: &lt;job stderr_log_level="error">.
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1329IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(getClass,
    errorLoggedIsTolerated = _.message contains "TEST-STDERR")

  private val processClassSetting = List(
    "Without Agent" → (() ⇒ ProcessClassConfiguration()),
    "With Agent" → (() ⇒ ProcessClassConfiguration(agentUris = List(agentUri))))
  private val jobSetting = List(
    JobPath("/test-exit-0")        → Set(ReturnCode(1)),  // stderr output changes ReturnCode(0) to ReturnCode(1)
    JobPath("/test-delay-exit-0")  → Set(ReturnCode(1)),  // stderr output changes ReturnCode(0) to ReturnCode(1)
    JobPath("/test-exit-77")       → Set(ReturnCode(77), ReturnCode(1)),  // Sometimes 77 (good), sometimes 1 (not so good) ???
    JobPath("/test-delay-exit-77") → Set(ReturnCode(77), ReturnCode(1)))  // Sometimes 77 (good), sometimes 1 (not so good) ???

  for ((groupName, processClass) ← processClassSetting;
       (jobPath, expectedReturnCodes) ← jobSetting) {
    s"$groupName, $jobPath, expecting $expectedReturnCodes" in {
      writeConfigurationFile(ProcessClassPath("/test"), processClass())
      val result = testOutput(jobPath)
      assert(expectedReturnCodes(result.returnCode))
    }
  }

  private def testOutput(jobPath: JobPath): TaskResult = {
    var result: TaskResult = null
    eventBus.awaitingWhen[InfoLogged](_.event.message contains "TEST-STDOUT") {
      eventBus.awaitingWhen[ErrorLogged](_.event.message contains "TEST-STDERR") {
        result = controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-280"))) {
          runJob(jobPath)
        }
      } .event.message should not include "[stderr]"
    } .event.message should not include "[stdout]"
    result
  }
}
