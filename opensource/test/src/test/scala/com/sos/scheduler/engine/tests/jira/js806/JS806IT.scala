package com.sos.scheduler.engine.tests.jira.js806

import JS806IT._
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.data.folder.FileBasedActivatedEvent
import com.sos.scheduler.engine.data.folder.JobChainPath
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS806IT extends FunSuite with ScalaSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(ignoreError = Set("SCHEDULER-280"))
  private lazy val orderSubsystem = instance[OrderSubsystem]

  test("A set back then reset order should respect changed configuration file") {
    autoClosing(controller.newEventPipe()) { eventPipe =>
      orderSubsystem.order(testOrderKey).getTitle shouldEqual aTitle
      scheduler executeXml ModifyOrderCommand(testOrderKey, at = Some(ModifyOrderCommand.NowAt))
      eventPipe.nextKeyed[OrderSetBackEvent](testOrderKey).state shouldEqual OrderState("200")
      eventPipe.nextKeyed[OrderStepEndedEvent](testOrderKey)

      xml.XML.save(testOrderKey.file(controller.environment.liveDirectory).getPath, <order title={bTitle}/>)
      scheduler executeXml ModifyOrderCommand(testOrderKey, action = Some(ModifyOrderCommand.Action.reset))

      eventPipe.nextKeyed[FileBasedActivatedEvent](testOrderKey)
      orderSubsystem.order(testOrderKey).getTitle shouldEqual bTitle
      eventPipe.nextKeyed[OrderTouchedEvent](testOrderKey)
      eventPipe.nextKeyed[OrderStateChangedEvent](testOrderKey).previousState shouldEqual OrderState("100")
    }
  }
}

private object JS806IT {
  private val testOrderKey = JobChainPath("/test") orderKey "1"
  private val aTitle = "AAA"
  private val bTitle = "BBB"
}
