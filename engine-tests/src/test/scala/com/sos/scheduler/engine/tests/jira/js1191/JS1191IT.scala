package com.sos.scheduler.engine.tests.jira.js1191

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.data.filebased.{FileBasedAddedEvent, FileBasedRemovedEvent}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderState, OrderStateChangedEvent}
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1191.JS1191IT._
import java.nio.file.{Files, Paths}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * JS-1191 Order.last_error
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1191IT extends FreeSpec with ScalaSchedulerTest {

  "Order.last_error" in {
    controller.toleratingErrorLogEvent(_ ⇒ true) {
      val result = runOrder(JobChainPath("/test") orderKey "1")
      assert(result.state == OrderState("END"))
    }
  }

  "Order.last_error survives a restart" in {
    controller.toleratingErrorLogEvent(_ ⇒ true) {
      scheduler executeXml <job_chain_node.modify job_chain="/test" state="NO-ERROR-2" action="stop"/>
      val orderKey = TestJobChainPath orderKey "2"
      eventBus.awaitingEvent[OrderStateChangedEvent](e ⇒ e.orderKey == orderKey && e.state == OrderState("NO-ERROR-2")) {
        startOrder(orderKey)
      }
      val file = testEnvironment.fileFromPath(TestJobChainPath)
      val renamed = Paths.get(s"$file~")
      eventBus.awaitingKeyedEvent[FileBasedRemovedEvent](TestJobChainPath) {
        Files.move(file, renamed)
      }
      eventBus.awaitingKeyedEvent[FileBasedAddedEvent](TestJobChainPath) {
        Files.move(renamed, file)
      }
      eventBus.awaitingKeyedEvent[OrderFinishedEvent](orderKey) {
        scheduler executeXml <job_chain_node.modify job_chain="/test" state="NO-ERROR-2" action="process"/>
      } .state shouldBe OrderState("END")
    }
  }
}

private object JS1191IT {
  private val TestJobChainPath = JobChainPath("/test")
}
