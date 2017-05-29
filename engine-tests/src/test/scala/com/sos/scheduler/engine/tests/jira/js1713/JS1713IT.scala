package com.sos.scheduler.engine.tests.jira.js1713

import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.OrderState
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * JS-1713 Respect end_state after order step error.
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1713IT extends FreeSpec with ScalaSchedulerTest {

  "Respect end_state after order step error" in {
    val orderKey = JobChainPath("/test") orderKey "1"
    val endState = OrderState("200")
    val result = controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-280"))) {
      runOrder(OrderCommand(orderKey, endState = Some(endState)))
    }
    assert(result.state == endState)
  }
}
