package com.sos.scheduler.engine.tests.jira.js1790

import com.sos.scheduler.engine.data.event.KeyedEvent
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderStepEnded, OrderSuspended}
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.test.SchedulerTestUtils.startOrder
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1790.JS1790IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/** JS-1790 <on_exit_code><to_state> means order was successful and on_error= is ignored.
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1790IT extends FreeSpec with ScalaSchedulerTest
{
  "JS-1790 <on_exit_code><to_state> means order was successful and on_error= is ignored" in {
    eventBus.on[OrderSuspended.type] {
      case KeyedEvent(orderKey, _) ⇒ scheduler executeXml OrderCommand(orderKey, suspended = Some(false), nodeId = Some(NodeId("SUSPENDED")))
    }
    controller.withEventPipe { eventPipe ⇒
      controller.withEventPipe { eventPipe2 ⇒
        controller.suppressingTerminateOnError {
          val orderRuns =
            List(
              "0" → SuspendJobChainPath, "1" → SuspendJobChainPath, "2" → SuspendJobChainPath,
              "0" → SetbackJobChainPath, "1" → SetbackJobChainPath, "2" → SetbackJobChainPath)
            .map { case (exitCode, jobChainPath) ⇒
              startOrder(OrderCommand(jobChainPath orderKey exitCode, parameters = Map("EXIT" → exitCode)))
            }
          val finished = eventPipe.next[OrderFinished](orderRuns map (_.orderKey))
          assert(finished.map(_.event.nodeId) ==
            NodeId("NEXT") :: NodeId("SUSPENDED-1") :: NodeId("200") ::
            NodeId("NEXT") :: NodeId("ERROR") :: NodeId("200") :: Nil)
          val ended = eventPipe2.queued[OrderStepEnded]
          assert(orderRuns.map(_.orderKey).map(orderKey ⇒ ended.count(_.key == orderKey)) ==
            1 :: 2 :: 1 ::
            1 :: 3 :: 1 :: Nil)
        }
      }
    }
  }
}

object JS1790IT {
  private val SuspendJobChainPath = JobChainPath("/suspend")
  private val SetbackJobChainPath = JobChainPath("/setback")
}
