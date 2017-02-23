package com.sos.scheduler.engine.tests.order.monitor.spoolertaskafter

import com.sos.jobscheduler.data.event.KeyedEvent
import com.sos.scheduler.engine.data.order.OrderFinished
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class SpoolerTaskAfterIT extends FunSuite with ScalaSchedulerTest {

  protected override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    terminateOnError = false)

  test("spooler_task_after should have access to last processed order") {
    scheduler executeXml <order job_chain="/test" id="1"/>
    controller.waitForTermination()
  }

  eventBus.onHot[OrderFinished] {
    case KeyedEvent(orderKey, _) ⇒
      orderDetailed(orderKey).variables(classOf[TestMonitor].getName) should equal ("exitCode=7")
      controller.terminateScheduler()
  }
}

