package com.sos.scheduler.engine.tests.jira.js1435

import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderState, OrderStateChangedEvent}
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1435.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1435IT extends FreeSpec with ScalaSchedulerTest {

  import controller.eventBus

  "JS-1435IT" in {
    runJobAndWaitForEnd(JobPath("/test"))
    val orderKey = JobChainPath("/test") orderKey "1"
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](orderKey) {
      val e = eventBus.awaitingKeyedEvent[OrderStateChangedEvent](orderKey) {
        scheduler executeXml OrderCommand(orderKey)
      }
      assert(e.previousState == OrderState("100"))
      assert(e.state == OrderState("END"))
    }
  }
}
