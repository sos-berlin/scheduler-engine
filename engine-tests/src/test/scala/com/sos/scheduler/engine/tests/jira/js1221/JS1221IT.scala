package com.sos.scheduler.engine.tests.jira.js1221

import com.google.common.io.Files.touch
import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.data.job.{JobPath, TaskStarted}
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.data.order.{OrderFinished, OrderStepStarted}
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
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
  private val fileGenerator: Iterator[File] = Iterator from 1 map { i ⇒ directory / s"test-$i" }

//  "Existing file" in {  DOES NOT WORK RELIABLE, because job chain cannot be created with next_state at once (atomic).
//    val file = fileGenerator.next()
//    check(file) {
//      createFile(file)
//      scheduler executeXml jobChainElem(directory)
//      scheduler executeXml <job_chain_node.modify job_chain="/test" state={AOrderState.string} action="next_state"/>
//    }
//  }

  "Added file" in {
    scheduler executeXml jobChainElem(directory)
    scheduler executeXml <job_chain_node.modify job_chain="/test" state={AOrderNodeId.string} action="next_state"/>
    val file = fileGenerator.next()
    checkFirstJobChainNodeIsSkipped(file) {
      createFile(file)
    }
  }

  def createFile(file: File): Unit = {
    onClose { file.delete() }
    touch(file)
  }

  private def checkFirstJobChainNodeIsSkipped(file: File)(body: ⇒ Unit): Unit = {
    val orderKey = TestJobChainPath.orderKey(file.getPath)
    withEventPipe { eventPipe ⇒
      body
      eventPipe.nextAny[TaskStarted.type].key.jobPath shouldEqual BJobPath
      eventPipe.next[OrderStepStarted](orderKey).nodeId shouldEqual BOrderNodeId
      eventPipe.next[OrderFinished](orderKey)
    }
  }
}

private object JS1221IT {
  private val TestJobChainPath = JobChainPath("/test")
  private val AOrderNodeId = NodeId("AAA")
  private val BOrderNodeId = NodeId("BBB")
  private val AJobPath = JobPath("/test-a")
  private val BJobPath = JobPath("/test-b")

  private def jobChainElem(directory: File) =
    <job_chain name="test">
      <file_order_source directory={directory.getPath}/>
      <job_chain_node state={AOrderNodeId.string} job={AJobPath.string}/>
      <job_chain_node state={BOrderNodeId.string} job={BJobPath.string}/>
      <file_order_sink state="SINK" remove="yes"/>
    </job_chain>
}
