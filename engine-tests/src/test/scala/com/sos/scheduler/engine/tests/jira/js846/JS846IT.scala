package com.sos.scheduler.engine.tests.jira.js846

import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderFinished
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js846.JS846IT._
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS846IT extends FunSuite with ScalaSchedulerTest {

  for (titleLength <- Seq(201, 10000)) {
    test(s"Order with title $titleLength characters should not lead to database error") {
      val orderKey = jobChainPath.orderKey("2")
      val eventPipe = controller.newEventPipe()
      val longTitle = "x" * titleLength
      scheduler executeXml <order job_chain={orderKey.jobChainPath.string} id={orderKey.id.string} title={longTitle}/>
      eventPipe.nextKeyed[OrderFinished](orderKey)
    }
  }

  test("Order with title growing longer than 200 characters should not lead to database error") {
    val orderKey = jobChainPath.orderKey("1")
    val eventPipe = controller.newEventPipe()
    scheduler executeXml <modify_order job_chain={orderKey.jobChainPath.string} order={orderKey.id.string} suspended="false"/>
    eventPipe.nextKeyed[OrderFinished](orderKey)
  }
}

private object JS846IT {
  val jobChainPath = JobChainPath("/test")
}
