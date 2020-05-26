package com.sos.scheduler.engine.tests.jira.js1877

import com.sos.scheduler.engine.common.scalautil.AutoClosing._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderFinished
import com.sos.scheduler.engine.data.schedule.SchedulePath
import com.sos.scheduler.engine.kernel.persistence.hibernate.HibernateOrderStore
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate.transaction
import com.sos.scheduler.engine.kernel.settings.{CppSettingName, CppSettings}
import com.sos.scheduler.engine.test.SchedulerTestUtils.{orderOverview, writeConfigurationFile}
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.{ImplicitTimeout, ProvidesTestEnvironment, TestSchedulerController}
import com.sos.scheduler.engine.tests.jira.js1877.JS1877IT._
import javax.persistence.EntityManagerFactory
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS1877IT extends FreeSpec
{
  private implicit val timeout = ImplicitTimeout(99.s)

  private lazy val testConfiguration =
    TestConfiguration(
      testClass = getClass,
      cppSettings = CppSettings.TestMap + (CppSettingName.alwaysCreateDatabaseTables -> false.toString))

  "After JobScheduler restart with an unchanged .order.xml, a previous state should remain" in {
    autoClosing(ProvidesTestEnvironment(testConfiguration)) { envProvider =>
      envProvider.runScheduler() { implicit controller =>
        assert(orderOverview(orderKey).nextStepAt == None)
        controller.withEventPipe { eventPipe =>
          controller.scheduler executeXml <modify_order job_chain="/TEST" order="TEST" at="now"/>
          eventPipe.next[OrderFinished](orderKey)
        }
        assert(orderOverview(orderKey).nextStepAt == None)
        checkDatabaseRecord("2038-01-19 03:14:07.000Z"/*never*/)

        writeConfigurationFile(
          SchedulePath("/OVERLAY"),
          <schedule valid_from="2020-04-01 00:00:00" substitute="/TEST">
            <period single_start="23:59"/>
          </schedule>)
        assert(orderOverview(orderKey).nextStepAt.isDefined)
        logger.info(orderOverview(orderKey).nextStepAt.toString)
        checkDatabaseRecord("23:59:00.000Z")
      }
      envProvider.runScheduler() { implicit controller =>
        checkDatabaseRecord("23:59:00.000Z")
        assert(orderOverview(orderKey).nextStepAt.isDefined)
        logger.info(orderOverview(orderKey).nextStepAt.toString)
      }
    }
  }

  private def checkDatabaseRecord(expectedTime: String)(implicit controller: TestSchedulerController): Unit =
    transaction(entityManagerFactory) { implicit entityManager =>
      val maybeXml = orderStore.fetch(orderKey).xmlOption
      if (!maybeXml.exists(_.contains(expectedTime + '"'))) {
        fail(s"Database record for order doest not have expected start time '$expectedTime': $maybeXml")
      }
    }

  private def entityManagerFactory(implicit controller: TestSchedulerController) =
    controller.instance[EntityManagerFactory]

  private def orderStore(implicit controller: TestSchedulerController) =
    controller.instance[HibernateOrderStore]
}


private object JS1877IT
{
  private val logger = Logger(getClass)
  private val orderKey = JobChainPath("/TEST") orderKey "TEST"
}
