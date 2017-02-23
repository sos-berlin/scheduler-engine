package com.sos.scheduler.engine.tests.jira.js1048

import com.sos.jobscheduler.common.scalautil.AutoClosing._
import com.sos.jobscheduler.common.scalautil.xmls.ScalaXmls.implicits._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.xmlcommands.{ModifyOrderCommand, OrderCommand}
import com.sos.scheduler.engine.kernel.persistence.hibernate.HibernateOrderStore
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate._
import com.sos.scheduler.engine.kernel.settings.{CppSettingName, CppSettings}
import com.sos.scheduler.engine.test.SchedulerTestUtils.{order, orderOption, orderOverview}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.{ProvidesTestEnvironment, TestEnvironment, TestSchedulerController}
import com.sos.scheduler.engine.tests.jira.js1048.JS1048IT._
import java.nio.file.Files
import javax.persistence.EntityManagerFactory
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS1048IT extends FreeSpec {

  private lazy val testConfiguration =
    TestConfiguration(
      testClass = getClass,
      cppSettings = CppSettings.TestMap + (CppSettingName.alwaysCreateDatabaseTables -> false.toString))

  "After JobScheduler restart with an unchanged .order.xml, a previous state should remain" in {
    autoClosing(ProvidesTestEnvironment(testConfiguration)) { envProvider =>
      envProvider.runScheduler() { implicit controller =>
        requireOriginalFileBasedOrders()
        modifyOrders()
      }
      envProvider.runScheduler() { implicit controller =>
        assert(orderOverview(SuspendOrderKey).isSuspended)
        order(TitleOrderKey).title shouldEqual CommandModifiedTitle
        requireDatabaseRecords(suspendedOrderExists = true, expectedTitle = Some(CommandModifiedTitle))
      }
    }
  }

  "After JobScheduler restart with a changed .order.xml, a previous state should be lost" in {
    autoClosing(ProvidesTestEnvironment(testConfiguration)) { envProvider =>
      envProvider.runScheduler() { implicit controller =>
        requireOriginalFileBasedOrders()
        modifyOrders()
      }
      SuspendOrderKey.file(envProvider.testEnvironment.liveDirectory).xml = <order><run_time/></order>
      TitleOrderKey.file(envProvider.testEnvironment.liveDirectory).xml = <order title={FileChangedTitle}><run_time/></order>
      envProvider.runScheduler() { implicit controller =>
        assert(!orderOverview(SuspendOrderKey).isSuspended)
        order(TitleOrderKey).title shouldEqual FileChangedTitle
        requireDatabaseRecords(suspendedOrderExists = false, expectedTitle = None)
      }
    }
  }

  "After JobScheduler restart with a removed .order.xml, the unmodified order should be removed" in {
    autoClosing(ProvidesTestEnvironment(testConfiguration)) { envProvider =>
      envProvider.runScheduler() { implicit controller =>
        requireOriginalFileBasedOrders()
      }
      removeOrderFiles(envProvider.testEnvironment)
      envProvider.runScheduler() { implicit controller =>
        requireOrdersNotExist()
        requireDatabaseRecords(suspendedOrderExists = false, expectedTitle = None)
      }
    }
  }

//  "After JobScheduler abort and restart with a removed .order.xml, the unmodified order should be removed" in {
//    pending
//    runScheduler { implicit controller =>
//      controller.abortScheduler()   Wir können den C++-Scheduler nicht abbrechen, außer mit exit(99).
//    }
//    removeOrderFiles()
//    runScheduler { implicit controller =>
//      ordersShouldNotExists()
//    }
//  }

  "After JobScheduler restart with a removed .order.xml, the modified order should be removed" in {
    autoClosing(ProvidesTestEnvironment(testConfiguration)) { envProvider =>
      envProvider.runScheduler() { implicit controller =>
        requireOriginalFileBasedOrders()
        modifyOrders()
      }
      removeOrderFiles(envProvider.testEnvironment)
      envProvider.runScheduler() { implicit controller =>
        requireOrdersNotExist()
        requireDatabaseRecords(suspendedOrderExists = false, expectedTitle = None)
      }
    }
  }

  "After JobScheduler restart with an added .order.xml (converting to a standing order), a previous state should be lost" in {
   pendingUntilFixed {  // FIXME Die neue .order.xml soll den Datenbanksatz ersetzen
    autoClosing(ProvidesTestEnvironment(testConfiguration)) { envProvider =>
      envProvider.runScheduler(activate = false) { implicit controller =>
        controller.prepare()
        removeOrderFiles(envProvider.testEnvironment)   // Die konfigurierten Test-Aufträge wollen wir hier nicht.
        controller.activateScheduler()
        requireOrdersNotExist()
        controller.scheduler executeXml OrderCommand(SuspendOrderKey, xmlChildren = <run_time/>)
        controller.scheduler executeXml OrderCommand(TitleOrderKey, title = Some(FileChangedTitle), xmlChildren = <run_time/>)
        assert(orderOverview(SuspendOrderKey).isSuspended)
        order(TitleOrderKey).title shouldEqual FileChangedTitle
      }
      SuspendOrderKey.file(envProvider.testEnvironment.liveDirectory).xml = <order><run_time/></order>
      TitleOrderKey.file(envProvider.testEnvironment.liveDirectory).xml = <order title={FileChangedTitle}><run_time/></order>
      envProvider.runScheduler() { implicit controller =>
        requireOriginalFileBasedOrders()
        requireDatabaseRecords(suspendedOrderExists = false, expectedTitle = None)
      }
    }
   }
  }

  private def modifyOrders()(implicit controller: TestSchedulerController): Unit = {
    controller.scheduler executeXml ModifyOrderCommand(SuspendOrderKey, suspended = Some(true))
    assert(orderOverview(SuspendOrderKey).isSuspended)
    order(TitleOrderKey).title shouldEqual OriginalTitle
    controller.scheduler executeXml ModifyOrderCommand(TitleOrderKey, title = Some(CommandModifiedTitle))
    order(TitleOrderKey).title shouldEqual CommandModifiedTitle
    requireDatabaseRecords(suspendedOrderExists = true, expectedTitle = Some(CommandModifiedTitle))
  }

  private def removeOrderFiles(testEnvironment: TestEnvironment): Unit = {
    for (o <- OrderKeys)
      Files.delete(o.file(testEnvironment.liveDirectory).toPath)
  }

  private def requireOriginalFileBasedOrders()(implicit controller: TestSchedulerController): Unit = {
    assert(!orderOverview(SuspendOrderKey).isSuspended)
    order(TitleOrderKey).title shouldEqual OriginalTitle
  }

  private def requireOrdersNotExist()(implicit controller: TestSchedulerController): Unit = {
    orderOption(SuspendOrderKey) should not be 'defined
    orderOption(TitleOrderKey) should not be 'defined
  }

  private def requireDatabaseRecords(suspendedOrderExists: Boolean, expectedTitle: Option[String])(implicit controller: TestSchedulerController): Unit = {
    transaction(entityManagerFactory) { implicit entityManager =>
      withClue(s"Existence of database record for $SuspendOrderKey:") {
        orderStore.tryFetch(SuspendOrderKey).isDefined shouldEqual suspendedOrderExists   // Wert von Suspended steckt irgendwo im XML. Prüfen wir nicht.
      }
      withClue(s"Existence of database record for $TitleOrderKey:") {
        orderStore.tryFetch(TitleOrderKey) map { _.title } shouldEqual expectedTitle
      }
    }
  }

  private def entityManagerFactory(implicit controller: TestSchedulerController) =
    controller.instance[EntityManagerFactory]

  private def orderStore(implicit controller: TestSchedulerController) =
    controller.instance[HibernateOrderStore]
}


private object JS1048IT {
  private val TestJobChainPath = JobChainPath("/test")
  private val TitleOrderKey = TestJobChainPath orderKey "TITLE"
  private val OriginalTitle = "ORIGINAL-TITLE"
  private val CommandModifiedTitle = "COMMAND-MODIFIED"
  private val FileChangedTitle = "FILE-CHANGED"
  private val SuspendOrderKey = TestJobChainPath orderKey "SUSPEND"
  private val OrderKeys = List(SuspendOrderKey, TitleOrderKey)
}
