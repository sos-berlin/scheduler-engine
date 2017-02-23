package com.sos.scheduler.engine.tests.jira.js628

import com.sos.jobscheduler.data.event.KeyedEvent
import com.sos.scheduler.engine.data.order.OrderFinished
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.test.util.CommandBuilder
import com.sos.scheduler.engine.tests.jira.js628.JS628IT.{TestJobChains, expectedErrorCount, expectedSuccessCount}
import org.junit.Assert.assertEquals
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * JS-628: Order successfull when a pre/postprocessing script is used
  * <p>
  * This test contain four jobchains for various combinations of the result from spooler_process and
  * spooler_process_before:
  * <pre>
  * spooler_process      spooler_process_before      result              job_chain
  * ==================   ========================    =================   =====================
  * true                 false                       error               js628_chain_fail_1
  * false                false                       error               js628_chain_fail_2
  * false                true                        error               js628_chain_fail_3
  * true                 true                        success             js628_chain_success
  * </pre>
  * It should be clarify that the job ends only if the tesult of spooler_process AND spooler_process_before
  * is true.
  * <p>
  * The test estimate that one job_chain ends with success and three job_chains ends with a failure.
  */
@RunWith(classOf[JUnitRunner])
final class JS628IT extends FreeSpec with ScalaSchedulerTest {
  private var finishedOrderCount: Int = 0
  private var errorCount: Int = 0
  private var successCount: Int = 0

  "test" in {
    val commandBuilder = new CommandBuilder
    for (jobChain ← TestJobChains) {
      val cmd: String = commandBuilder.addOrder(jobChain).getCommand
      controller.scheduler.executeXml(cmd)
    }
    controller.waitForTermination()
    assertEquals("total number of events", TestJobChains.length, finishedOrderCount)
    assertEquals("successfull orders", expectedSuccessCount, successCount)
    assertEquals("unsuccessfull orders", expectedErrorCount, errorCount)
  }

  eventBus.onHot[OrderFinished] {
    case KeyedEvent(orderKey, _) ⇒
      val order = orderOverview(orderKey)
      val endState = order.nodeId.string
      if (endState == "error") { errorCount += 1; errorCount - 1 }
      if (endState == "success") { successCount += 1; successCount - 1 }
      finishedOrderCount += 1
      if (finishedOrderCount == TestJobChains.length) controller.terminateScheduler()
    }
}

private object JS628IT {
  private val TestJobChains = Array("js628-chain-success-1", "js628-chain-success-2", "js628-chain-fail-1", "js628-chain-fail-2", "js628-chain-fail-3", "js628-chain-fail-4")
  private val expectedErrorCount = 4
  private val expectedSuccessCount = 2
}
