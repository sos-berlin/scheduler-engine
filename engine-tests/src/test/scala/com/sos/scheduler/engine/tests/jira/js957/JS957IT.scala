package com.sos.scheduler.engine.tests.jira.js957

import com.sos.jobscheduler.common.scalautil.AutoClosing._
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderKey}
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand
import com.sos.scheduler.engine.kernel.order.OrderSubsystemClient
import com.sos.scheduler.engine.kernel.settings.{CppSettingName, CppSettings}
import com.sos.scheduler.engine.test.configuration.{DefaultDatabaseConfiguration, TestConfiguration}
import com.sos.scheduler.engine.test.{ProvidesTestEnvironment, TestSchedulerController}
import com.sos.scheduler.engine.tests.jira.js957.JS957IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/** Jira-Tickets JS-956, JS-957 und JS-1172. */
@RunWith(classOf[JUnitRunner])
final class JS957IT extends FreeSpec {

  private val testConfiguration = TestConfiguration(
    testClass = getClass,
    logCategories = "scheduler java.stackTrace-",
    cppSettings = CppSettings.TestMap + (CppSettingName.alwaysCreateDatabaseTables -> false.toString),
    database = Some(DefaultDatabaseConfiguration(closeDelay = 60.s)))

  "Order should start once, and after Scheduler abort and restart, order should continue starting" in {
    autoClosing(ProvidesTestEnvironment(testConfiguration)) { envProvider ⇒
      envProvider.runScheduler() { implicit controller ⇒
        autoClosing(controller.newEventPipe()) { eventPipe ⇒
          controller.instance[OrderSubsystemClient].order(RepeatOrderKey)
          controller.scheduler executeXml ModifyOrderCommand(RepeatOrderKey, title = Some(AlteredTitle))
          repeatOrder.title shouldEqual AlteredTitle
          eventPipe.next[OrderFinished](RepeatOrderKey)
          eventPipe.next[OrderFinished](RepeatOrderKey)
          executeShowOrder().toString should include ("<source")
          simulateAbort()
        }
      }
      envProvider.runScheduler() { implicit controller ⇒
        autoClosing(controller.newEventPipe()) { eventPipe ⇒
          repeatOrder.title shouldEqual AlteredTitle
          eventPipe.next[OrderFinished](RepeatOrderKey)
          eventPipe.next[OrderFinished](RepeatOrderKey)
          eventPipe.next[OrderFinished](RepeatOrderKey)
          executeShowOrder().toString should include ("<source")   // JS-956: Nach Wiederherstellung des Auftrags aus der Datenbank wird weiterhin der Text der Konfigurationsdatei geliefert
          controller.terminateScheduler()
        }
      }
    }
  }

  private def simulateAbort()(implicit controller: TestSchedulerController): Unit = {
    autoClosing(controller.newJDBCConnection()) { connection ⇒
      val statement = connection.createStatement()
      statement execute """alter table "SCHEDULER_ORDERS" rename to  "SCHEDULER_ORDERS_FREEZED" """
      statement execute """create table "SCHEDULER_ORDERS" as select * from "SCHEDULER_ORDERS_FREEZED" """
      controller.terminateScheduler()
      controller.waitForTermination()
      statement execute """drop table "SCHEDULER_ORDERS" """
      statement execute """alter table "SCHEDULER_ORDERS_FREEZED" rename to  "SCHEDULER_ORDERS" """
    }
  }

  private def executeShowOrder()(implicit controller: TestSchedulerController) =
    controller.scheduler executeXml
      <show_order job_chain={RepeatOrderKey.jobChainPath.string} order={RepeatOrderKey.id.string} what="source"/>

  private def repeatOrder(implicit controller: TestSchedulerController) =
    controller.injector.getInstance(classOf[OrderSubsystemClient]).order(RepeatOrderKey)
}


private object JS957IT {
  private val RepeatOrderKey = OrderKey("/test1", "orderWithRepeat")
  private val AlteredTitle = "ALTERED TITLE"
}
