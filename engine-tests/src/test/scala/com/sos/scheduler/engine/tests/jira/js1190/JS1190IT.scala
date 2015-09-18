package com.sos.scheduler.engine.tests.jira.js1190

import com.sos.scheduler.engine.data.job.{JobPath, ReturnCode}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.{ErrorOrderStateTransition, OrderState, OrderStateChangedEvent, OrderStateTransition, OrderStepEndedEvent, ProceedingOrderStateTransition, SuccessOrderStateTransition}
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
    for ((exitCode, expectedState) ← ExitCodeToState) {
      s"Exit code $exitCode" in {
        runAndCheckOrder("SIMPLE", Map(), exitCode, ProceedingOrderStateTransition(ReturnCode(exitCode)), expectedState)
      }
    }
  }

  "Shell job with monitor" - {
    "(replace job)" in {
      scheduler executeXml MonitorShellJobElem
    }
    for (((beforeProcess, exitCode, afterProcess), (expectedTransition, expectedState)) ← MonitorExitCodeToState) {
      s"spooler_process_before=$beforeProcess, exit code $exitCode, spooler_process_after=$afterProcess" in {
        runAndCheckOrder("MONITOR",
          Map(BeforeProcessParam → beforeProcess.toString, AfterProcessParam → afterProcess.toString),
          exitCode, expectedTransition, expectedState)
      }
    }
  }

  private def runAndCheckOrder(prefix: String, parameters: Map[String, String], exitCode: Int,
      expectedTransition: OrderStateTransition, expectedState: OrderState): Unit =
  {
    val orderKey = TestJobchainPath orderKey s"$prefix-EXIT-$exitCode"
    val orderCommand = OrderCommand(orderKey, parameters = Map("EXIT_CODE" → s"$exitCode") ++ parameters)
    controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-280"), MessageCode("SCHEDULER-226"))) {
      withEventPipe { eventPipe ⇒
        scheduler executeXml orderCommand
        eventPipe.nextKeyed[OrderStepEndedEvent](orderKey).stateTransition shouldEqual expectedTransition
        eventPipe.nextKeyed[OrderStateChangedEvent](orderKey).state shouldEqual expectedState
      }
    }
  }
}

private object JS1190IT {
  private val TestJobchainPath = JobChainPath("/test")
  private val ExitCodeToState = List(
    0 → OrderState("STATE-0"),
    1 → OrderState("STATE-1"),
    99 → OrderState("STATE-99"),
    100 → OrderState("ERROR"))

  private val MonitorExitCodeToState = List[((Boolean, Int, Boolean), (OrderStateTransition, OrderState))](
    (true, 0, true) → (SuccessOrderStateTransition, OrderState("STATE-0")),
    (true, 1, false) → (ErrorOrderStateTransition(ReturnCode(1)), OrderState("STATE-1")),
    (true, 99, false) → (ErrorOrderStateTransition(ReturnCode(99)), OrderState("STATE-99")),
    (true, 100, false) → (ErrorOrderStateTransition(ReturnCode(100)), OrderState("ERROR")),

    (true, 0, false) → (ErrorOrderStateTransition.Standard, OrderState("STATE-1")),
    (true, 1, true) → (SuccessOrderStateTransition, OrderState("STATE-0")),
    (true, 99, true) → (SuccessOrderStateTransition, OrderState("STATE-0")),
    (true, 100, true) → (SuccessOrderStateTransition, OrderState("STATE-0")),

    (false, 0, true) → (ErrorOrderStateTransition.Standard, OrderState("STATE-1")))

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
