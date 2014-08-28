package com.sos.scheduler.engine.tests.jira.js578

import JS578IT._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.test.EventPipe
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS578IT extends FunSuite with ScalaSchedulerTest {
  private lazy val orderSubsystem = scheduler.injector.getInstance(classOf[OrderSubsystem])

  test("<modify_order at='now'/>") {
    val eventPipe = controller.newEventPipe()
    startOrderAt("now")
    eventPipe.nextWithCondition[OrderFinishedEvent] { _.orderKey == orderKey }
  }

  test("<modify_order at='now'/> while order is running does nothing") {
    val eventPipe = controller.newEventPipe()
    setJobChainNodeStop(true)
    startOrderAt("now")
    eventPipe.nextWithCondition[OrderStepEndedEvent] { _.orderKey == orderKey }
    orderSubsystem.order(orderKey).state should equal (OrderState("200"))

    startOrderAt("now")
    setJobChainNodeStop(false)
    eventPipe.nextWithCondition[OrderFinishedEvent] { _.orderKey == orderKey }
    intercept[EventPipe.TimeoutException] { eventPipe.nextWithTimeoutAndCondition[OrderTouchedEvent](3.s)  { _.orderKey == orderKey }}
  }

  ignore("<modify_order at='next'/> (PENDING)") {
    pendingUntilFixed {
      val eventPipe = controller.newEventPipe()
      startOrderAt("next")
      eventPipe.nextWithCondition[OrderFinishedEvent] { _.orderKey == orderKey }
    }
  }

  ignore("<modify_order at='next'/> while order is running repeats order (PENDING)") {
    pendingUntilFixed {
      val eventPipe = controller.newEventPipe()
      setJobChainNodeStop(true)
      startOrderAt("next")
      eventPipe.nextWithCondition[OrderStepEndedEvent] { _.orderKey == orderKey }
      orderSubsystem.order(orderKey).state should equal (OrderState("200"))

      startOrderAt("next")
      setJobChainNodeStop(false)
      eventPipe.nextWithCondition[OrderFinishedEvent] { _.orderKey == orderKey }
      eventPipe.nextWithCondition[OrderTouchedEvent] { _.orderKey == orderKey }
      eventPipe.nextWithCondition[OrderFinishedEvent] { _.orderKey == orderKey }
    }
  }

  private def startOrderAt(at: String): Unit = {
    scheduler executeXml <modify_order job_chain={orderKey.jobChainPath.string} order={orderKey.id.string} at={at}/>
  }

  private def setJobChainNodeStop(b: Boolean): Unit = {
    scheduler executeXml <job_chain_node.modify job_chain={orderKey.jobChainPath.string} state="200" action={if (b) "stop" else "process"}/>
  }
}

private object JS578IT {
  private val orderKey = OrderKey("/test", "1")
}
