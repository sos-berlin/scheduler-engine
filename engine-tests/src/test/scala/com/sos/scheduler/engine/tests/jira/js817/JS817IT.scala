package com.sos.scheduler.engine.tests.jira.js817

import com.sos.jobscheduler.data.event.KeyedEvent
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderId, OrderKey}
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js817.JS817IT.TestOrderId
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * JS-801 "JobScheduler crashes if order_state is set to a delayed jobchain_node by api".
  *
  * @see <a href="http://www.sos-berlin.com/jira/browse/JS-801">JS-801</a>
  */
@RunWith(classOf[JUnitRunner])
final class JS817IT extends FreeSpec with ScalaSchedulerTest {

  "JobChain without delay shell" in {
    runSchedulerWithJobChain("simple_chain")
  }

  private def runSchedulerWithJobChain(jobChainName: String) {
    scheduler.executeXml(<order job_chain={jobChainName} id={TestOrderId}/>)
  }

  eventBus.on[OrderFinished] {
    case KeyedEvent(OrderKey(_, TestOrderId), _)  ⇒
      controller.terminateScheduler()
  }
}

private object JS817IT {
  private val TestOrderId = OrderId("test")
}
