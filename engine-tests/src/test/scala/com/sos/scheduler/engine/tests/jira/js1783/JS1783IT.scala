package com.sos.scheduler.engine.tests.jira.js1783

import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.order.OrderFinished
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1783IT extends FreeSpec with ScalaSchedulerTest
{
  override protected lazy val testConfiguration = TestConfiguration(getClass, terminateOnError = false)

  "JS-1783 a" in {
    pending  // eventBus.awaiting times out?
    val orderKey = JobChainPath("/a") orderKey "start"
    val finished = eventBus.awaiting[OrderFinished](orderKey) {
      scheduler executeXml <modify_order job_chain="/a/job_chain6" order="start" at="now"/>
    }
    assert(finished.nodeId == NodeId("SUCCESS"))
  }

  "JS-1783 b first node has on_error=suspend" in {
    pending  // eventBus.awaiting times out?
    val orderKey = JobChainPath("/b") orderKey "start"
    val finished = eventBus.awaiting[OrderFinished](orderKey) {
      scheduler executeXml <modify_order job_chain="/b/job_chain6" order="start" at="now"/>
    }
    assert(finished.nodeId == NodeId("SUCCESS"))
  }
}
