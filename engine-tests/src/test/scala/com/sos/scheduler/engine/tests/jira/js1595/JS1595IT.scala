package com.sos.scheduler.engine.tests.jira.js1595

import com.sos.jobscheduler.common.system.OperatingSystem._
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.DotnetProvidingAgent
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
  * JS-1595 Powershell, JS-1623 VBScript.
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1595IT extends FreeSpec with ScalaSchedulerTest with DotnetProvidingAgent {

  "JavaScript, as reference" in {
    testOrder(JobChainPath("/test-javascript"))
  }

  if (isWindows) {
    for (language ← List("PowerShell" /*FIXME, "VBScript"*/))
      s"$language" - {
        val jobChainPath = JobChainPath(s"/test-${language.toLowerCase}")
        "Script" in {
          testOrder(jobChainPath).nodeId shouldEqual NodeId("END")
        }

        "Exception in script" in {
          controller.toleratingErrorCodes(Set(MessageCode("COM-80020009"))) {
            testOrder(jobChainPath, Map("FAIL" → "1")).nodeId shouldEqual NodeId("FAILED")
          }
        }
      }
  }

  private def testOrder(jobChainPath: JobChainPath, parameters: Map[String, String] = Map()): OrderRunResult = {
    val p = Map("TEST" → "test-value") ++ parameters
    val result = runOrder(OrderCommand(jobChainPath orderKey "1", parameters = p))
    assert(result.variables == p + ("NEW-TEST" → "test-value"))
    result
  }
}
