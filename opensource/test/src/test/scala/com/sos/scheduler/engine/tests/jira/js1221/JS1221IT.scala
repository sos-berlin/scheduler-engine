package com.sos.scheduler.engine.tests.jira.js1221

import com.google.common.io.Files.touch
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.data.job.{JobPath, TaskStartedEvent}
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderState, OrderStepStartedEvent}
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.tests.jira.js1221.JS1221IT._
import java.io.File
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/**
 * JS-1221 &lt;job_chain_nody.modify action="next_state"> auf ersten Knoten nach &lt;file_order_source>.
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1221IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val directory = testEnvironment.newFileOrderSourceDirectory()

  "Existing file" in {
    val file = newFile()
    scheduler executeXml jobChainElem(directory)
    scheduler executeXml <job_chain_node.modify job_chain="/test" state={AOrderState.string} action="next_state"/>
    check(file)
  }

  "Added file" in {
    val file = newFile()
    check(file)
  }

  private object newFile {
    private val i = Iterator.from(1)

    def apply() = {
      val file = directory / s"test-${i.next()}"
      onClose { file.delete() }
      touch(file)
      file
    }
  }

  private def check(file: File): Unit = {
    val orderKey = TestJobChainPath.orderKey(file.getPath)
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      eventPipe.nextAny[TaskStartedEvent].jobPath shouldEqual BJobPath
      eventPipe.nextKeyed[OrderStepStartedEvent](orderKey).state shouldEqual BOrderState
      eventPipe.nextKeyed[OrderFinishedEvent](orderKey)
    }
  }
}

private object JS1221IT {
  private val TestJobChainPath = JobChainPath("/test")
  private val AOrderState = OrderState("AAA")
  private val BOrderState = OrderState("BBB")
  private val AJobPath = JobPath("/test-a")
  private val BJobPath = JobPath("/test-b")

  private def jobChainElem(directory: File) =
    <job_chain name="test">
      <file_order_source directory={directory.getPath}/>
      <job_chain_node state={AOrderState.string} job={AJobPath.string}/>
      <job_chain_node state={BOrderState.string} job={BJobPath.string}/>
      <file_order_sink state="SINK" remove="yes"/>
    </job_chain>
}
