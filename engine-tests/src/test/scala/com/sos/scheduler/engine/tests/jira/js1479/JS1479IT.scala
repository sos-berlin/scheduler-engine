package com.sos.scheduler.engine.tests.jira.js1479

import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderTouchedEvent}
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.concurrent.Future

/**
 * JS-1479 For last job node in a job chain &lt;copy_params from="order"> does not work

 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1479IT extends FreeSpec with ScalaSchedulerTest {

  "copy_params in first and in last job chain node" in {
    val primaryOrderKey = JobChainPath("/test-a") orderKey "1"
    val eventPipe = controller.newEventPipe()
    scheduler executeXml OrderCommand(primaryOrderKey, parameters = Map("A" → "TEST-A"))
    assert(eventPipe.nextAny[OrderTouchedEvent].orderKey == primaryOrderKey)
    val addedOrderFinishedSeq = List.fill(2) {
      val orderKey = eventPipe.nextAny[OrderTouchedEvent].orderKey
      eventBus.keyedEventFuture[OrderFinishedEvent](orderKey)
    }
    awaitSuccess(Future.sequence(addedOrderFinishedSeq))
  }
}
