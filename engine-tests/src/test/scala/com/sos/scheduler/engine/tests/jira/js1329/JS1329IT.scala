package com.sos.scheduler.engine.tests.jira.js1329

import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.job.{JobPath, ReturnCode}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.log.{ErrorLogEvent, InfoLogEvent}
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.OrderState
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.data.xmlcommands.ProcessClassConfiguration
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.DotnetProvidingAgent
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
  * JS-1329, JS-1615, JS-1734: &lt;job stderr_log_level="error">.
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1329IT extends FreeSpec with ScalaSchedulerTest with DotnetProvidingAgent {

  override protected lazy val testConfiguration = TestConfiguration(getClass,
    errorLogEventIsTolerated = _ ⇒ true)
    //errorLogEventIsTolerated = e ⇒ Set("TEST-STDERR", "API-TEST-ERROR")(e.message))

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

  for ((groupName, processClass) ← processClassSetting) {  // JS-1734 output to stderr should let order change to error_state: Job test-exit-0
    s"$groupName, order" in {
      writeConfigurationFile(ProcessClassPath("/test"), processClass())
      val setting = List(
        JobChainPath("/test-without-stderr") → OrderState("END"),
        JobChainPath("/test-exit-0") → OrderState("ERROR"),
        JobChainPath("/test-api") → OrderState("END"))  // In an API job, spooler_log.error() does not let move order to error_state
      val jobChainPaths = setting map (_._1)
      val whenResults = for (jobChainPath ← jobChainPaths) yield startOrder(jobChainPath orderKey "1").result
      val nodeIds = whenResults await 99.s map (_.state)
      assert(jobChainPaths.zip(nodeIds) == setting)
    }
  }

  if (isWindows)
    "Powershell write-error" in {
      val result = startOrder(JobChainPath("/test-powershell") orderKey "1").result await 99.s
      assert(result.state == OrderState("ERROR"))
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
