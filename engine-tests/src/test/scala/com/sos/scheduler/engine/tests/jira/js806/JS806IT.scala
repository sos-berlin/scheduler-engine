package com.sos.scheduler.engine.tests.jira.js806

import com.sos.jobscheduler.common.scalautil.xmls.ScalaXmls.implicits._
import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.log.{InfoLogged, Logged}
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.data.filebased.FileBasedActivated
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand
import com.sos.scheduler.engine.kernel.variable.SchedulerVariableSet
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js806.JS806IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/** JS-806 Orders in setback can not be changed. */
@RunWith(classOf[JUnitRunner])
final class JS806IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val liveDirectory = controller.environment.liveDirectory
  private lazy val variableSet = instance[SchedulerVariableSet]

  "Change of order configuration file while order is set back should be effective when order has been reset" in {
    val myOrderKey = SetbackJobChainPath orderKey "A"
    withEventPipe { eventPipe ⇒
      variableSet("TestJob.setback") = true.toString
      order(myOrderKey).title shouldEqual OriginalTitle
      scheduler executeXml ModifyOrderCommand.startNow(myOrderKey)
      eventPipe.next[OrderSetBack](myOrderKey).nodeId shouldEqual NodeId("200")
      eventPipe.next[OrderStepEnded](myOrderKey)

      (liveDirectory resolve myOrderKey.xmlFile).xml = <order title={ChangedTitle}><run_time/></order>
      scheduler executeXml ModifyOrderCommand(myOrderKey, action = Some(ModifyOrderCommand.Action.reset))

      eventPipe.next[FileBasedActivated.type](myOrderKey)
      order(myOrderKey).title shouldEqual ChangedTitle
    }
  }

  "Change of order configuration file while order is suspended should be effective when order has been reset" in {
    val jobChainPath = SuspendingJobChainPath
    val myOrderKey = jobChainPath orderKey "A"
    withEventPipe { eventPipe ⇒
      order(myOrderKey).title shouldEqual OriginalTitle
      scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state="200" action="stop"/>
      scheduler executeXml ModifyOrderCommand.startNow(myOrderKey)
      eventPipe.next[OrderStepEnded](myOrderKey)
      eventPipe.next[OrderNodeChanged](myOrderKey).nodeIdTransition shouldEqual NodeId("100") -> NodeId("200")
      (liveDirectory resolve myOrderKey.xmlFile).xml = <order title={ChangedTitle}><run_time/></order>
      scheduler executeXml ModifyOrderCommand(myOrderKey, suspended = Some(true))
      eventPipe.nextWhen[InfoLogged] { _.event.codeOption == Some(MessageCode("SCHEDULER-991")) }    // "Order has been suspended"
      scheduler executeXml ModifyOrderCommand(myOrderKey, action = Some(ModifyOrderCommand.Action.reset))
      eventPipe.nextWhen[InfoLogged] { _.event.codeOption == Some(MessageCode("SCHEDULER-992")) }    // "Order ist not longer suspended"
      eventPipe.next[FileBasedActivated.type](myOrderKey)
      order(myOrderKey).title shouldEqual ChangedTitle
      scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state="200" action="process"/>
    }
  }

  "Change of order configuration file while order is running should be effective when order is finished" in {
    val jobChainPath = StoppingJobChainPath
    val myOrderKey = jobChainPath orderKey "A"
    withEventPipe { eventPipe ⇒
      order(myOrderKey).title shouldEqual OriginalTitle
      scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state="200" action="stop"/>
      scheduler executeXml ModifyOrderCommand.startNow(myOrderKey)
      eventPipe.next[OrderStepEnded](myOrderKey)
      (liveDirectory resolve myOrderKey.xmlFile).xml = <order title={ChangedTitle}><run_time/></order>
      eventPipe.nextWhen[Logged] { _.event.codeOption == Some(MessageCode("SCHEDULER-892")) }   // This Standing_order is going to be replaced due to changed configuration file ...
      scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state="200" action="process"/>
      eventPipe.next[OrderFinished](myOrderKey)
      eventPipe.next[FileBasedActivated.type](myOrderKey)
      order(myOrderKey).title shouldEqual ChangedTitle
    }
  }

  "Change of order configuration file while order is set back should be effective when order is finished" in {
    // Wie vorheriger Test, aber komplizierter mit setback
    val myOrderKey = SetbackJobChainPath orderKey "B"
    withEventPipe { eventPipe ⇒
      variableSet("TestJob.setback") = true.toString
      order(myOrderKey).title shouldEqual OriginalTitle
      scheduler executeXml ModifyOrderCommand.startNow(myOrderKey)
      eventPipe.next[OrderSetBack](myOrderKey).nodeId shouldEqual NodeId("200")
      eventPipe.next[OrderStepEnded](myOrderKey)
      (liveDirectory resolve myOrderKey.xmlFile).xml = <order title={ChangedTitle}><run_time/></order>
      variableSet("TestJob.setback") = false.toString
      eventPipe.next[OrderFinished](myOrderKey)
      eventPipe.next[FileBasedActivated.type](myOrderKey)
      order(myOrderKey).title shouldEqual ChangedTitle
    }
  }
}

private object JS806IT {
  private val SetbackJobChainPath = JobChainPath("/test-setback")
  private val StoppingJobChainPath = JobChainPath("/test-stop")
  private val SuspendingJobChainPath = JobChainPath("/test-suspend")
  private val OriginalTitle = "TITLE"
  private val ChangedTitle = "CHANGED"
}
