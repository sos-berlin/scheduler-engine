package com.sos.scheduler.engine.tests.jira.js1598

import com.sos.jobscheduler.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderKey, OrderStarted, OrderSuspended}
import com.sos.scheduler.engine.data.xmlcommands.{ModifyOrderCommand, OrderCommand}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1598.JS1598IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * JS-1598 &lt;modify_order suspended="false"> does not work with a distributed job chain.
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1598IT extends FreeSpec with ScalaSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List("-distributed-orders"),
    ignoreError = Set(MessageCode("SCHEDULER-280")))

  "A Job chain node with attribut suspend=true" - {
    "Permanent order" in {
      checkRun(AOrderKey) { orderKey ⇒
        scheduler executeXml ModifyOrderCommand(orderKey, at = Some(ModifyOrderCommand.NowAt))
      }
    }

    "Non-permanent order" in {
      checkRun(AJobChainPath orderKey "1") { orderKey ⇒
        scheduler executeXml OrderCommand(orderKey)
      }
    }
  }

  "B Job chain node with attribut on_error=suspend" - {
    "Permanent order" in {
      checkRun(BOrderKey, continueNodeId = Some(NodeId("continue"))) { orderKey ⇒
        scheduler executeXml ModifyOrderCommand(orderKey, at = Some(ModifyOrderCommand.NowAt))
      }
    }

    "Non-permanent order" in {
      checkRun(BJobChainPath orderKey "1", continueNodeId = Some(NodeId("continue"))) { orderKey ⇒
        scheduler executeXml OrderCommand(orderKey)
      }
    }
  }

  private def checkRun(orderKey: OrderKey, continueNodeId: Option[NodeId] = None)(body: OrderKey ⇒ Unit): Unit =
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      body(orderKey)
      eventPipe.next[OrderStarted.type](orderKey)
      eventPipe.next[OrderSuspended.type](orderKey)
      scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(false), nodeId = continueNodeId)
      // Not for a distributed order: eventPipe.next[OrderResumed](orderKey)
      eventPipe.next[OrderFinished](orderKey)
    }
}

private object JS1598IT {
  private val AJobChainPath = JobChainPath("/test-a")
  private val BJobChainPath = JobChainPath("/test-b")
  private val AOrderKey = AJobChainPath orderKey "permanent"
  private val BOrderKey = BJobChainPath orderKey "permanent"
}
