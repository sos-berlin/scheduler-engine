package com.sos.scheduler.engine.tests.jira.js1008

import com.google.common.io.Files.touch
import com.sos.scheduler.engine.data.jobchain.{JobChainNodeAction, JobChainPath}
import com.sos.scheduler.engine.data.order.{OrderState, OrderStepEnded}
import com.sos.scheduler.engine.kernel.order.OrderSubsystemClient
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1008.JS1008IT._
import java.io.File
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS1008IT extends FreeSpec with ScalaSchedulerTest {

  "file_order_source" in {
    // Ob Fehler in FindFirstChangeNotification ignoriert werden, testen wir manuell durch vorübergehenden Eingriff in den C++-Code. Dazu Polling-Intervall mit repeat="5" verkürzen
    val eventPipe = controller.newEventPipe()
    val orderSubsystem = instance[OrderSubsystemClient]
    val directory = testEnvironment.newFileOrderSourceDirectory()
    scheduler executeXml jobChainElem(directory)
    orderSubsystem.jobChain(testJobChainPath).node(OrderState("200")).action = JobChainNodeAction.stop
    val file = new File(directory, "test")
    touch(file)
    eventPipe.nextWithCondition[OrderStepEnded] { _.orderKey == testJobChainPath.orderKey(file.getPath) }
  }
}

object JS1008IT {
  private val testJobChainPath = JobChainPath("/test")

  private def jobChainElem(directory: File) =
    <job_chain name="test">
      <file_order_source directory={directory.getPath}/>
      <job_chain_node state="100" job="test"/>
      <job_chain_node state="200" job="test"/>
      <job_chain_node.end state="end"/>
    </job_chain>
}
