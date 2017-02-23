package com.sos.scheduler.engine.tests.jira.js1190

import com.sos.jobscheduler.data.job.ReturnCode
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.order.{OrderNodeChanged, OrderNodeTransition, OrderStepEnded}
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1190.JS1190IT._
import com.sos.scheduler.engine.tests.jira.js1190.TestMonitor.{AfterProcessParam, BeforeProcessParam}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1190IT extends FreeSpec with ScalaSchedulerTest {

  "Simple shell job" - {
    "(add job)" in {
      scheduler executeXml SimpleShellJobElem
    }
    for ((exitCode, expectedState) ← ExitCodeToNodeId) {
      s"Exit code $exitCode" in {
        runAndCheckOrder("SIMPLE", Map(), exitCode, OrderNodeTransition.Proceeding(ReturnCode(exitCode)), expectedState)
      }
    }
  }

  "Shell job with monitor" - {
    "(replace job)" in {
      scheduler executeXml MonitorShellJobElem
    }
    for (((beforeProcess, exitCode, afterProcess), (expectedTransition, expectedState)) ← MonitorExitCodeToNodeId) {
      s"spooler_process_before=$beforeProcess, exit code $exitCode, spooler_process_after=$afterProcess" in {
        runAndCheckOrder("MONITOR",
          Map(BeforeProcessParam → beforeProcess.toString, AfterProcessParam → afterProcess.toString),
          exitCode, expectedTransition, expectedState)
      }
    }
  }

  private def runAndCheckOrder(prefix: String, parameters: Map[String, String], exitCode: Int,
      expectedTransition: OrderNodeTransition, expectedState: NodeId): Unit =
  {
    val orderKey = TestJobchainPath orderKey s"$prefix-EXIT-$exitCode"
    val orderCommand = OrderCommand(orderKey, parameters = Map("EXIT_CODE" → s"$exitCode") ++ parameters)
    controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-280"), MessageCode("SCHEDULER-226"))) {
      withEventPipe { eventPipe ⇒
        scheduler executeXml orderCommand
        eventPipe.next[OrderStepEnded](orderKey).nodeTransition shouldEqual expectedTransition
        eventPipe.next[OrderNodeChanged](orderKey).nodeId shouldEqual expectedState
      }
    }
  }
}

private object JS1190IT {
  private val TestJobchainPath = JobChainPath("/test")
  private val ExitCodeToNodeId = List(
    0 → NodeId("STATE-0"),
    1 → NodeId("STATE-1"),
    99 → NodeId("STATE-99"),
    100 → NodeId("ERROR"))

  private val MonitorExitCodeToNodeId = List[((Boolean, Int, Boolean), (OrderNodeTransition, NodeId))](
    ((true, 0, true), (OrderNodeTransition.Success, NodeId("STATE-0"))),
    ((true, 1, false), (OrderNodeTransition.Error(ReturnCode(1)), NodeId("STATE-1"))),
    ((true, 99, false), (OrderNodeTransition.Error(ReturnCode(99)), NodeId("STATE-99"))),
    ((true, 100, false), (OrderNodeTransition.Error(ReturnCode(100)), NodeId("ERROR"))),
    ((true, 0, false), (OrderNodeTransition.Error.Standard, NodeId("STATE-1"))),
    ((true, 1, true), (OrderNodeTransition.Success, NodeId("STATE-0"))),
    ((true, 99, true), (OrderNodeTransition.Success, NodeId("STATE-0"))),
    ((true, 100, true), (OrderNodeTransition.Success, NodeId("STATE-0"))),
    ((false, 0, true), (OrderNodeTransition.Error.Standard, NodeId("STATE-1"))))

  private val TestJobPath = JobPath("/test-a")
  private val JobScriptElem =
    <script language="shell">
      :;if false; then :
      goto WINDOWS
      fi
      exit $SCHEDULER_PARAM_EXIT_CODE

      :WINDOWS
      exit %SCHEDULER_PARAM_EXIT_CODE%
    </script>

  private val SimpleShellJobElem = <job name={TestJobPath.name} stop_on_error="false">{JobScriptElem}</job>

  private val MonitorShellJobElem =
    <job name={TestJobPath.name} replace="yes" stop_on_error="false">
      {JobScriptElem}
      <monitor>
        <script java_class="com.sos.scheduler.engine.tests.jira.js1190.TestMonitor"/>
      </monitor>
    </job>
}
