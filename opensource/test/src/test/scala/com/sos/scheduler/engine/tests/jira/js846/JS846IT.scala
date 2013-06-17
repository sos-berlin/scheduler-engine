package com.sos.scheduler.engine.tests.jira.js846

import JS846IT._
import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.order.OrderFinishedEvent
import com.sos.scheduler.engine.test.configuration.{DefaultDatabaseConfiguration, TestConfiguration}
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
class JS846IT extends ScalaSchedulerTest {

  override lazy val testConfiguration = TestConfiguration(
    database = Some(DefaultDatabaseConfiguration()))

  for (titleLength <- Seq(201, 500)) {   // Bei 10000 Zeichen gbt es einen StackOverflowError
    test(s"Order with title $titleLength characters should not lead to database error") {
      val orderKey = jobChainPath.orderKey("2")
      val eventPipe = controller.newEventPipe()
      val longTitle = "x" * titleLength
      scheduler executeXml <order job_chain={orderKey.jobChainPathString} id={orderKey.idString} title={longTitle}/>
      eventPipe.nextWithCondition { e: OrderFinishedEvent => e.orderKey == orderKey }
    }
  }

  test("Order with title growing longer than 200 characters should not lead to database error") {
    val orderKey = jobChainPath.orderKey("1")
    val eventPipe = controller.newEventPipe()
    scheduler executeXml <modify_order job_chain={orderKey.jobChainPathString} order={orderKey.idString} suspended="false"/>
    eventPipe.nextWithCondition { e: OrderFinishedEvent => e.orderKey == orderKey }
  }
}

private object JS846IT {
  val jobChainPath = JobChainPath.of("/test")
}
