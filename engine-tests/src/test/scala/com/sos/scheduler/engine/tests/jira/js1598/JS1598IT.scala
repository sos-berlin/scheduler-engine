package com.sos.scheduler.engine.tests.jira.js1598

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderKey, OrderState, OrderSuspended, OrderStarted}
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
      checkRun(BOrderKey, continueState = Some(OrderState("continue"))) { orderKey ⇒
        scheduler executeXml ModifyOrderCommand(orderKey, at = Some(ModifyOrderCommand.NowAt))
      }
    }

    "Non-permanent order" in {
      checkRun(BJobChainPath orderKey "1", continueState = Some(OrderState("continue"))) { orderKey ⇒
        scheduler executeXml OrderCommand(orderKey)
      }
    }
  }

  private def checkRun(orderKey: OrderKey, continueState: Option[OrderState] = None)(body: OrderKey ⇒ Unit): Unit =
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      body(orderKey)
      eventPipe.nextKeyed[OrderStarted](orderKey)
      eventPipe.nextKeyed[OrderSuspended](orderKey)
      scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(false), state = continueState)
      // Not for a distributed order: eventPipe.nextKeyed[OrderResumed](orderKey)
      eventPipe.nextKeyed[OrderFinished](orderKey)
    }
}

private object JS1598IT {
  private val AJobChainPath = JobChainPath("/test-a")
  private val BJobChainPath = JobChainPath("/test-b")
  private val AOrderKey = AJobChainPath orderKey "permanent"
  private val BOrderKey = BJobChainPath orderKey "permanent"
}
