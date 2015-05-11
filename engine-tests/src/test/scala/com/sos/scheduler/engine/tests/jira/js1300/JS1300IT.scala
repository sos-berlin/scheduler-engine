package com.sos.scheduler.engine.tests.jira.js1300

import com.google.common.io.Files.touch
import com.sos.scheduler.engine.agent.test.AgentTest
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderFinishedEvent
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1300.JS1300IT._
import java.io.File
import java.nio.file.Files
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1300 <file_order_source remote_scheduler="..">
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1300IT extends FreeSpec with ScalaSchedulerTest with AgentTest {

  private val directory = testEnvironment.newFileOrderSourceDirectory()

  "JS-1300" in {
    scheduler executeXml newJobChainElem(directory, agentUri = agentUri)
    for (_ ← 1 to 3) {
      val file = Files.createTempFile(directory, "test", ".tmp")
      val orderKey = TestJobChainPath orderKey file.getPath
      eventBus.awaitingKeyedEvent[OrderFinishedEvent](orderKey) {
        touch(file)
      }
    }
  }
}

private object JS1300IT {
  private val TestJobChainPath = JobChainPath("/test")

  private def newJobChainElem(directory: File, agentUri: String): xml.Elem =
    <job_chain name={TestJobChainPath.withoutStartingSlash}>
      <file_order_source directory={directory.getPath} remote_scheduler={agentUri} repeat="10"/>
      <job_chain_node state="100" job="/test"/>
      <job_chain_node.end state="END"/>
    </job_chain>
}
