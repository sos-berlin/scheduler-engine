package com.sos.scheduler.engine.tests.jira.js1048

import JS1048IT._
import com.sos.scheduler.engine.common.scalautil.ScalaXmls.implicits._
import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand
import com.sos.scheduler.engine.kernel.order.{Order, OrderSubsystem}
import com.sos.scheduler.engine.test.configuration.{DefaultDatabaseConfiguration, TestConfiguration}
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.test.scalatest.HasCloserBeforeAndAfterAll
import com.sos.scheduler.engine.test.{ProvidesTestEnvironment, TestSchedulerController}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS1048IT
    extends FreeSpec
    with HasCloserBeforeAndAfterAll
    with ProvidesTestEnvironment {

  override lazy val testConfiguration =
    TestConfiguration(database = Some(DefaultDatabaseConfiguration()))

  private def order(key: OrderKey)(implicit controller: TestSchedulerController): Order =
    controller.instance[OrderSubsystem].order(key)

  "After JobScheduler restart with an unchanged .order.xml, a previous modification should remain" in {
    runScheduler { implicit controller =>
      controller.scheduler executeXml ModifyOrderCommand(suspendOrderKey, suspended = Some(true))
      order(suspendOrderKey).isSuspended shouldBe true
      order(titleOrderKey).title shouldEqual originalTitle
      controller.scheduler executeXml ModifyOrderCommand(titleOrderKey, title = Some(commandModifiedTitle))
      order(titleOrderKey).title shouldEqual commandModifiedTitle
    }

    runScheduler { implicit controller =>
      order(suspendOrderKey).isSuspended shouldBe true
      order(titleOrderKey).title shouldEqual commandModifiedTitle
    }
  }

  "After JobScheduler restart with a changed .order.xml, a previous modification should be lost" in {
    runScheduler { implicit controller =>
      controller.scheduler executeXml ModifyOrderCommand(suspendOrderKey, suspended = Some(true))
      order(titleOrderKey).title shouldEqual originalTitle
      controller.scheduler executeXml ModifyOrderCommand(titleOrderKey, title = Some(commandModifiedTitle))
      order(titleOrderKey).title shouldEqual commandModifiedTitle
    }

    suspendOrderKey.file(testEnvironment.liveDirectory).xml = <order><run_time/></order>
    titleOrderKey.file(testEnvironment.liveDirectory).xml = <order title={fileChangedTitle}><run_time/></order>

    runScheduler { implicit controller =>
      order(suspendOrderKey).isSuspended shouldBe false
      order(titleOrderKey).title shouldEqual fileChangedTitle
    }
  }
}


private object JS1048IT {
  private val testJobChainPath = JobChainPath("/test")
  private val titleOrderKey = testJobChainPath orderKey "TITLE"
  private val originalTitle = "ORIGINAL-TITLE"
  private val commandModifiedTitle = "COMMAND-MODIFIED"
  private val fileChangedTitle = "FILE-CHANGED"
  private val suspendOrderKey = testJobChainPath orderKey "SUSPEND"
}
