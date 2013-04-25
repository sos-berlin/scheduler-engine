package com.sos.scheduler.engine.tests.jira.js957

import JS957IT._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderId, OrderKey, OrderFinishedEvent}
import com.sos.scheduler.engine.eventbus.EventHandlerAnnotated
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.test.SchedulerTest.shortTimeout
import com.sos.scheduler.engine.test.TestSchedulerController
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import org.scalatest.{BeforeAndAfter, FunSuite, OneInstancePerTest}
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._

/** Jira-Tickets JS-956 und JS-957. */
@RunWith(classOf[JUnitRunner])
class JS957IT extends FunSuite with OneInstancePerTest with BeforeAndAfter with EventHandlerAnnotated {

  lazy val controller =
    TestSchedulerController.builder(getClass)
    .logCategories("scheduler java.stackTrace-")
    .build

  before {
    controller.useDatabase(60.s)
  }

  after {
    controller.close()
  }

  test("Order should start once") {
    val eventPipe = controller.newEventPipe()
    controller.activateScheduler()
    controller.scheduler.injector.getInstance(classOf[OrderSubsystem]).order(repeatOrderKey)
    repeatOrder.setTitle(alteredTitle)
    repeatOrder.getTitle should equal (alteredTitle)
    eventPipe.nextWithCondition[OrderFinishedEvent] { _.getKey == repeatOrderKey }
    eventPipe.nextWithCondition[OrderFinishedEvent] { _.getKey == repeatOrderKey }
    executeShowOrder().toString should include ("<source")
    simulateAbort()
  }

  private def simulateAbort() {
    val connection = controller.newJDBCConnection()
    try {
      val statement = connection.createStatement()
      statement execute """alter table "SCHEDULER_ORDERS" rename to  "SCHEDULER_ORDERS_FREEZED" """
      statement execute """create table "SCHEDULER_ORDERS" as select * from "SCHEDULER_ORDERS_FREEZED" """
      controller.terminateScheduler()
      controller.waitForTermination(shortTimeout)
      statement execute """drop table "SCHEDULER_ORDERS" """
      statement execute """alter table "SCHEDULER_ORDERS_FREEZED" rename to  "SCHEDULER_ORDERS" """
    }
    finally connection.close()
  }

  test("After Scheduler restart, order should continue starting") {
    val eventPipe = controller.newEventPipe()
    controller.activateScheduler()
    repeatOrder.getTitle should equal (alteredTitle)
    eventPipe.nextWithCondition[OrderFinishedEvent] { _.getKey == repeatOrderKey }
    eventPipe.nextWithCondition[OrderFinishedEvent] { _.getKey == repeatOrderKey }
    eventPipe.nextWithCondition[OrderFinishedEvent] { _.getKey == repeatOrderKey }
    executeShowOrder().toString should not include ("<source")   // Nach Wiederherstellung des Auftrags aus der Datenbank gibt es keine Quelle
    controller.terminateScheduler()
  }

  private def executeShowOrder() =
    controller.scheduler executeXml
          <show_order job_chain={repeatOrderKey.jobChainPathString} order={repeatOrderKey.idString} what="source"/>

  private def repeatOrder =
    controller.scheduler.injector.getInstance(classOf[OrderSubsystem]).order(repeatOrderKey)
}

private object JS957IT {
  val repeatOrderKey = new OrderKey(JobChainPath.of("/test1"), new OrderId("orderWithRepeat"))
  val alteredTitle = "ALTERED TITLE"
}
