package com.sos.scheduler.engine.tests.jira.js1195

import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderFinishedEvent
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1195.JS1195IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS1195IT extends FreeSpec with ScalaSchedulerTest  {

  import controller.eventBus

  "JS1195IT" in {
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](ClonedOrderKey) {
      eventBus.awaitingKeyedEvent[OrderFinishedEvent](OriginalOrderKey) {
        scheduler executeXml OrderCommand(OriginalOrderKey)
      }
    }
  }
}

private object JS1195IT {
  private val OriginalOrderKey = JobChainPath("/test-a") orderKey "TEST"
  private val ClonedOrderKey = JobChainPath("/test-b") orderKey "TEST"   // Added by <TestPlugin:clone_order TestPlugin:job_chain="/test-b"/>
}
