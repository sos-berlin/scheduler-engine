package com.sos.scheduler.engine.tests.jira.js731

import com.sos.jobscheduler.data.event.KeyedEvent
import com.sos.scheduler.engine.data.order.OrderFinished
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.hamcrest.Matchers.equalTo
import org.junit.Assert.assertThat
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/** @see <a href='http://www.sos-berlin.com/jira/browse/JS-731'>JS-731</a> */
@RunWith(classOf[JUnitRunner])
final class JS731IT extends FreeSpec with ScalaSchedulerTest {

  "testOrderParametersNamesAndGet" in {
    val params: String = "<params><param name='a' value='ä'/><param name='B' value='B'/></params>"
    scheduler.executeXml("<add_order job_chain='a' id='1'>" + params + "</add_order>")
    controller.waitForTermination()
  }

  eventBus.onHot[OrderFinished] {
    case KeyedEvent(orderKey, _) ⇒
      val v = orderDetailed(orderKey).variables
      assertThat(v("a"), equalTo("ä"))
      assertThat(v("B"), equalTo("B"))
      controller.terminateScheduler()
  }
}
