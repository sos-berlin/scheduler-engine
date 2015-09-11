package com.sos.scheduler.engine.tests.jira.js1395

import com.sos.scheduler.engine.data.job.{JobPath, TaskEndedEvent, TaskStartedEvent}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.OrderFinishedEvent
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1395.JS1395IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1395 FIX: &lt;job_chain_node.modify action="next_state"> does not work properly while the node is executing an order.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1395IT extends FreeSpec with ScalaSchedulerTest {

  "action=next_state does work properly even when the node has just executed the order" in {
    val orderKey = TestJobChainPath orderKey "1"
    controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-280"))) {
      eventBus.awaitingKeyedEvent[OrderFinishedEvent](orderKey) {
        eventBus.awaitingEvent[TaskEndedEvent](_.jobPath == ErrorJobPath) {
          eventBus.awaitingEvent[TaskStartedEvent](_.jobPath == ErrorJobPath) {
            scheduler executeXml OrderCommand(orderKey)
          }
          scheduler executeXml <job_chain_node.modify job_chain={TestJobChainPath.string} state="100" action="next_state"/>
        }
      }
    }
  }
}

private object JS1395IT {
  private val ErrorJobPath = JobPath("/test-error")
  private val TestJobChainPath = JobChainPath("/test")
}
