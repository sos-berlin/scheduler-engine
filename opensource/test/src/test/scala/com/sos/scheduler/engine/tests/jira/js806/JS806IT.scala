package com.sos.scheduler.engine.tests.jira.js806

import JS806IT._
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.ScalaXmls.implicits._
import com.sos.scheduler.engine.data.filebased.FileBasedActivatedEvent
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.log.LogEvent
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.kernel.variable.VariableSet
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/** JS-806 Orders in setback can not be changed. */
@RunWith(classOf[JUnitRunner])
final class JS806IT extends FunSuite with ScalaSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    ignoreError = Set(MessageCode("SCHEDULER-280")))

  private lazy val liveDirectory = controller.environment.liveDirectory
  private lazy val orderSubsystem = instance[OrderSubsystem]
  private lazy val variableSet = instance[VariableSet]

  test("Change of order configuration file while order is set back should be effective when order has been reset") {
    val myOrderKey = setbackJobChainPath orderKey "A"
    autoClosing(controller.newEventPipe()) { eventPipe =>
      variableSet("TestJob.setback") = true.toString
      orderSubsystem.order(myOrderKey).title shouldEqual originalTitle
      scheduler executeXml ModifyOrderCommand.startNow(myOrderKey)
      eventPipe.nextKeyed[OrderSetBackEvent](myOrderKey).state shouldEqual OrderState("200")
      eventPipe.nextKeyed[OrderStepEndedEvent](myOrderKey)

      myOrderKey.file(liveDirectory).xml = <order title={changedTitle}><run_time/></order>
      scheduler executeXml ModifyOrderCommand(myOrderKey, action = Some(ModifyOrderCommand.Action.reset))

      eventPipe.nextKeyed[FileBasedActivatedEvent](myOrderKey)
      orderSubsystem.order(myOrderKey).title shouldEqual changedTitle
    }
  }

  test("Change of order configuration file while order is running should be effective when order is finished") {
    val myOrderKey = suspendJobChainPath orderKey "A"
    autoClosing(controller.newEventPipe()) { eventPipe =>
      orderSubsystem.order(myOrderKey).title shouldEqual originalTitle
      scheduler executeXml <job_chain_node.modify job_chain={suspendJobChainPath.string} state="200" action="stop"/>
      scheduler executeXml ModifyOrderCommand.startNow(myOrderKey)
      eventPipe.nextKeyed[OrderStepEndedEvent](myOrderKey)
      myOrderKey.file(liveDirectory).xml = <order title={changedTitle}><run_time/></order>
      eventPipe.nextWithCondition[LogEvent] { e => Option(e.getCodeOrNull) == Some("SCHEDULER-892") }   // This Standing_order is going to be replaced due to changed configuration file ...
      scheduler executeXml <job_chain_node.modify job_chain={suspendJobChainPath.string} state="200" action="process"/>
      eventPipe.nextKeyed[OrderFinishedEvent](myOrderKey)
      eventPipe.nextKeyed[FileBasedActivatedEvent](myOrderKey)
      orderSubsystem.order(myOrderKey).title shouldEqual changedTitle
    }
  }

  test("Change of order configuration file while order is set back should be effective when order is finished") {
    // Wie vorheriger Test, aber komplizierter mit setback
    val myOrderKey = setbackJobChainPath orderKey "B"
    autoClosing(controller.newEventPipe()) { eventPipe =>
      variableSet("TestJob.setback") = true.toString
      orderSubsystem.order(myOrderKey).title shouldEqual originalTitle
      scheduler executeXml ModifyOrderCommand.startNow(myOrderKey)
      eventPipe.nextKeyed[OrderSetBackEvent](myOrderKey).state shouldEqual OrderState("200")
      eventPipe.nextKeyed[OrderStepEndedEvent](myOrderKey)
      myOrderKey.file(liveDirectory).xml = <order title={changedTitle}><run_time/></order>
      variableSet("TestJob.setback") = false.toString
      eventPipe.nextKeyed[OrderFinishedEvent](myOrderKey)
      eventPipe.nextKeyed[FileBasedActivatedEvent](myOrderKey)
      orderSubsystem.order(myOrderKey).title shouldEqual changedTitle
    }
  }
}

private object JS806IT {
  private val setbackJobChainPath = JobChainPath("/test-setback")
  private val suspendJobChainPath = JobChainPath("/test-suspend")
  private val originalTitle = "TITLE"
  private val changedTitle = "CHANGED"
}
