package com.sos.scheduler.engine.tests.jira.js957

import JS957IT._
import com.sos.scheduler.engine.common.scalautil.AutoClosing._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.order.OrderFinishedEvent
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.test.SchedulerTest.shortTimeout
import com.sos.scheduler.engine.test.configuration.{DefaultDatabaseConfiguration, TestConfiguration}
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.test.{ProvidesTestEnvironment, TestSchedulerController}
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/** Jira-Tickets JS-956 und JS-957. */
@RunWith(classOf[JUnitRunner])
final class JS957IT extends FunSuite {

  private val testConfiguration = TestConfiguration(
    testClass = getClass,
    logCategories = "scheduler java.stackTrace-",
    database = Some(DefaultDatabaseConfiguration(closeDelay = 60.s)))

  test("Order should start once, and after Scheduler abort and restart, order should continue starting") {
    autoClosing(ProvidesTestEnvironment(testConfiguration)) { envProvider =>
      envProvider.runScheduler() { implicit controller =>
        autoClosing(controller.newEventPipe()) { eventPipe =>
          controller.scheduler.injector.getInstance(classOf[OrderSubsystem]).order(repeatOrderKey)
          controller.scheduler executeXml ModifyOrderCommand(repeatOrderKey, title = Some(alteredTitle))
          repeatOrder.title shouldEqual alteredTitle
          eventPipe.nextKeyed[OrderFinishedEvent](repeatOrderKey)
          eventPipe.nextKeyed[OrderFinishedEvent](repeatOrderKey)
          executeShowOrder().toString should include ("<source")
          simulateAbort()
        }
      }
      envProvider.runScheduler() { implicit controller =>
        autoClosing(controller.newEventPipe()) { eventPipe =>
          repeatOrder.title shouldEqual alteredTitle
          eventPipe.nextKeyed[OrderFinishedEvent](repeatOrderKey)
          eventPipe.nextKeyed[OrderFinishedEvent](repeatOrderKey)
          eventPipe.nextKeyed[OrderFinishedEvent](repeatOrderKey)
          executeShowOrder().toString should include ("<source")   // JS-956: Nach Wiederherstellung des Auftrags aus der Datenbank wird weiterhin der Text der Konfigurationsdatei geliefert
          controller.terminateScheduler()
        }
      }
    }
  }

  private def simulateAbort()(implicit controller: TestSchedulerController) {
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

  private def executeShowOrder()(implicit controller: TestSchedulerController) =
    controller.scheduler executeXml
          <show_order job_chain={repeatOrderKey.jobChainPath.string} order={repeatOrderKey.id.string} what="source"/>

  private def repeatOrder(implicit controller: TestSchedulerController) =
    controller.scheduler.injector.getInstance(classOf[OrderSubsystem]).order(repeatOrderKey)
}


private object JS957IT {
  private val repeatOrderKey = OrderKey("/test1", "orderWithRepeat")
  private val alteredTitle = "ALTERED TITLE"
}
