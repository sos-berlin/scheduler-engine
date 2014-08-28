package com.sos.scheduler.engine.tests.order.monitor.spoolertaskafter

import com.sos.scheduler.engine.data.order.OrderFinishedEvent
import com.sos.scheduler.engine.eventbus.HotEventHandler
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
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
    controller.waitForTermination(shortTimeout)
  }

  @HotEventHandler def handleEvent(e: OrderFinishedEvent, order: UnmodifiableOrder): Unit = {
    order.parameters(classOf[TestMonitor].getName) should equal ("exitCode=7")
    controller.terminateScheduler()
  }
}
