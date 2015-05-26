package com.sos.scheduler.engine.tests.jira.js1300

import com.google.common.io.Files.touch
import com.sos.scheduler.engine.agent.test.AgentTest
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.log.InfoLogEvent
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderTouchedEvent}
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1300.JS1300IT._
import java.nio.file.Files.delete
import java.nio.file.Path
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

  "Some files, one after the other" in {
    scheduler executeXml newJobChainElem(directory, agentUri = agentUri, JobPath("/test-delete"))
    for (_ ← 1 to 3) {
      val file = directory / "TEST-DELETE"
      val orderKey = TestJobChainPath orderKey file.toString
      eventBus.awaitingEvent[InfoLogEvent](_.codeOption contains MessageCode("SCHEDULER-981")) {
        eventBus.awaitingKeyedEvent[OrderFinishedEvent](orderKey) {
          touch(file)
        }
      }
    }
  }

  "A file, not deleted by job chain, stays on blacklist" in {
    scheduler executeXml newJobChainElem(directory, agentUri = agentUri, JobPath("/test-dont-delete"))
    val file = directory / "TEST-DONT-DELETE"
    val orderKey = TestJobChainPath orderKey file.toString
    eventBus.awaitingEvent[InfoLogEvent](_.codeOption contains MessageCode("SCHEDULER-981")) {
      eventBus.awaitingKeyedEvent[OrderFinishedEvent](orderKey) {
        touch(file)
      }
      val startedAgain = eventBus.keyedEventFuture[OrderTouchedEvent](orderKey)
      assert(order(orderKey).isOnBlacklist)
      sleep(Repeat + Delay)
      assert(order(orderKey).isOnBlacklist)
      assert(!startedAgain.isCompleted)
      delete(file)
    }
    assert(!orderExists(orderKey))
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](orderKey) {
      val started = eventBus.keyedEventFuture[OrderTouchedEvent](orderKey)
      assert(!started.isCompleted)
      sleep(Repeat + Delay)
      assert(!started.isCompleted)
      touch(file)
    }
    assert(order(orderKey).isOnBlacklist)
  }
}

private object JS1300IT {
  private val TestJobChainPath = JobChainPath("/test")
  private val Repeat = 1.s
  private val Delay = Repeat dividedBy 2

  private def newJobChainElem(directory: Path, agentUri: String, jobPath: JobPath): xml.Elem =
    <job_chain name={TestJobChainPath.withoutStartingSlash}>
      <file_order_source directory={directory.toString} remote_scheduler={agentUri} repeat={Repeat.getSeconds.toString}/>
      <job_chain_node state="100" job={jobPath.string}/>
      <job_chain_node.end state="END"/>
    </job_chain>
}
