package com.sos.scheduler.engine.tests.jira.js1479

import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderStarted}
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.concurrent.Future

/**
 * JS-1479 For last job node in a job chain &lt;copy_params from="order"> does not work

 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1479IT extends FreeSpec with ScalaSchedulerTest {

  "FAILING: copy_params in first and in last job chain node" in {
    val primaryOrderKey = JobChainPath("/test-a") orderKey "1"
    val eventPipe = controller.newEventPipe()
    writeConfigurationFile(primaryOrderKey, OrderCommand(primaryOrderKey, parameters = Map("A" → "TEST-A")))
    assert(eventPipe.nextAny[OrderStarted.type].key == primaryOrderKey)
    val addedOrderFinishedSeq = List.fill(2) {
      val orderKey = eventPipe.nextAny[OrderStarted.type].key
      eventBus.eventFuture[OrderFinished](orderKey)
    }
    val nodeIds = awaitSuccess(Future.sequence(addedOrderFinishedSeq)) map { _.nodeId }
    intercept[Exception] {
      nodeIds shouldEqual List(NodeId("OKAY"), NodeId("OKAY"))
    }
    nodeIds shouldEqual List(NodeId("FAILED"), NodeId("FAILED"))  // Test fails with scheduler.order.keep_order_content_on_reschedule=false
  }
}
