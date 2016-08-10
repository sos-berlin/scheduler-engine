package com.sos.scheduler.engine.tests.jira.js986

import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.xmlcommands.{OrderCommand, ShowOrderCommand, What}
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js986.JS986IT._
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS986IT extends FunSuite with ScalaSchedulerTest {

  test("JS-986 Fix: order.nodeId=end should not suppress stdout in order log") {
    val eventPipe = controller.newEventPipe()
    scheduler executeXml OrderCommand(testOrderKey)
    eventPipe.nextWithCondition { e: OrderFinished => e.orderKey == testOrderKey }
    withClue("Task log") { controller.environment.taskLogFileString(testJobPath) should include (expectedJobOutput) }
    withClue("Order log") { orderLogString(testOrderKey) should include (expectedJobOutput) }
  }

  private def orderLogString(orderKey: OrderKey): String =
    ((scheduler executeXml ShowOrderCommand(testOrderKey, what=List(What.Log)).xmlElem).answer \ "order" \ "log").text
}

object JS986IT {
  private val expectedJobOutput = "*** EXPECTED JOB OUTPUT ***"
  private val testJobPath = JobPath("/test")
  private val testOrderKey = JobChainPath("/test").orderKey("1")
}
