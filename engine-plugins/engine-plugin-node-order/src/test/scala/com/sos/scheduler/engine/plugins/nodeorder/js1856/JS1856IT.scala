package com.sos.scheduler.engine.plugins.nodeorder.js1856

import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.OrderAdded
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils.runOrder
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS1856IT extends FreeSpec with ScalaSchedulerTest
{
  override protected lazy val testConfiguration = TestConfiguration(getClass,
    logCategories = "scheduler scheduler.mainlog",
    ignoreError = Set(MessageCode("SCHEDULER-280")))

  "Bug: Moving order with spooler_task.order.state=... activates the NextOrderPlugin of this new node" in {
    val orderKey = JobChainPath("/test") orderKey "A"
    val secondOrderKey = JobChainPath("/second") orderKey "SECOND"
    eventBus.awaiting[OrderAdded](secondOrderKey) {
      assert(runOrder(orderKey).nodeId == NodeId("200-ERROR"))
    }
  }
}
