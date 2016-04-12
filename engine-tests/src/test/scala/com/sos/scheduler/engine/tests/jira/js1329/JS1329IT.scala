package com.sos.scheduler.engine.tests.jira.js1329

import com.sos.scheduler.engine.data.job.{JobPath, ReturnCode}
import com.sos.scheduler.engine.data.log.{ErrorLogEvent, InfoLogEvent}
import com.sos.scheduler.engine.data.message.MessageCode
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
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1329IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(getClass,
    errorLogEventIsTolerated = _.message contains "TEST-STDERR")

  private val processClassSetting = List(
    "Without Agent" → (() ⇒ ProcessClassConfiguration()),
    "With Agent" → (() ⇒ ProcessClassConfiguration(agentUris = List(agentUri))))
  private val jobSetting = List(
    JobPath("/test-exit-0") → ReturnCode(1),       // stderr output changes ReturnCode(0) to ReturnCode(1)
    JobPath("/test-delay-exit-0") → ReturnCode(1), // stderr output changes ReturnCode(0) to ReturnCode(1)
    JobPath("/test-exit-77") → ReturnCode(77),
    JobPath("/test-delay-exit-77") → ReturnCode(77))

  for ((groupName, processClass) ← processClassSetting;
       (jobPath, expectedReturnCode) ← jobSetting) {
    s"$groupName, $jobPath, expecting $expectedReturnCode" in {
      writeConfigurationFile(ProcessClassPath("/test"), processClass())
      val result = testOutput(jobPath)
      assert(result.returnCode == expectedReturnCode)
    }
  }

  private def testOutput(jobPath: JobPath): TaskResult = {
    var result: TaskResult = null
    eventBus.awaitingEvent[InfoLogEvent](_.message contains "TEST-STDOUT") {
      eventBus.awaitingEvent[ErrorLogEvent](_.message contains "TEST-STDERR") {
        result = controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-280"))) {
          runJob(jobPath)
        }
      } .message should not include "[stderr]"
    } .message should not include "[stdout]"
    result
  }
}
