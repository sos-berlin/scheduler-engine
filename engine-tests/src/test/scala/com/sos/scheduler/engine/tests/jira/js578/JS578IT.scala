package com.sos.scheduler.engine.tests.jira.js578

import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.scheduler.engine.data.jobchain.NodeId
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.kernel.order.OrderSubsystemClient
import com.sos.scheduler.engine.test.EventPipe
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js578.JS578IT._
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS578IT extends FunSuite with ScalaSchedulerTest {
  private lazy val orderSubsystem = injector.getInstance(classOf[OrderSubsystemClient])

  test("<modify_order at='now'/>") {
    val eventPipe = controller.newEventPipe()
    startOrderAt("now")
    eventPipe.next[OrderFinished](orderKey)
  }

  test("<modify_order at='now'/> while order is running does nothing") {
    val eventPipe = controller.newEventPipe()
    setJobChainNodeStop(true)
    startOrderAt("now")
    eventPipe.next[OrderStepEnded](orderKey)
    orderSubsystem.orderOverview(orderKey).nodeId should equal (NodeId("200"))

    startOrderAt("now")
    setJobChainNodeStop(false)
    eventPipe.next[OrderFinished](orderKey)
    intercept[EventPipe.TimeoutException] {
      eventPipe.next[OrderStarted.type](orderKey, 3.s)
    }
  }

  ignore("<modify_order at='next'/> (PENDING)") {
    pendingUntilFixed {
      val eventPipe = controller.newEventPipe()
      startOrderAt("next")
      eventPipe.next[OrderFinished](orderKey)
    }
  }

  ignore("<modify_order at='next'/> while order is running repeats order (PENDING)") {
    pendingUntilFixed {
      val eventPipe = controller.newEventPipe()
      setJobChainNodeStop(true)
      startOrderAt("next")
      eventPipe.next[OrderStepEnded](orderKey)
      orderSubsystem.orderOverview(orderKey).nodeId should equal (NodeId("200"))

      startOrderAt("next")
      setJobChainNodeStop(false)
      eventPipe.next[OrderFinished](orderKey)
      eventPipe.next[OrderStarted.type](orderKey)
      eventPipe.next[OrderFinished](orderKey)
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
