package com.sos.scheduler.engine.tests.jira.js1435

import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderNodeChanged}
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

  "JS-1435IT" in {
    runJob(JobPath("/test"))
    val orderKey = JobChainPath("/test") orderKey "1"
    val event = eventBus.awaitingKeyedEvent[OrderFinished](orderKey) {
      val e = eventBus.awaitingKeyedEvent[OrderNodeChanged](orderKey) {
        scheduler executeXml OrderCommand(orderKey)
      }
      assert(e.fromNodeId == NodeId("100"))
      assert(e.nodeId == NodeId("END"))
    }
    assert(event.nodeId == NodeId("END"))
  }
}
