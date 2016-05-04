package com.sos.scheduler.engine.tests.jira.js1627

import com.google.common.io.Files._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.SideEffect.ImplicitSideEffect
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.OrderState
import com.sos.scheduler.engine.test.SchedulerTestUtils.{OrderRun, awaitSuccess}
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1627.JS1627IT._
import java.io.File
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1627IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val directory = testEnvironment.newFileOrderSourceDirectory()

  "file_order_sink does not change order state" in {
    val file = directory / "TEST"
    val orderKey = TestJobChainPath orderKey file.toString
    scheduler executeXml jobChainElem(directory)
    val run = OrderRun(orderKey) sideEffect { _ ⇒
      touch(file)
    }
    val state = awaitSuccess(run.result).state
    assert(state == SinkState)
  }
}

private object JS1627IT {
  private val TestJobChainPath = JobChainPath("/test")
  private val SinkState = OrderState("SINK")

  private def jobChainElem(directory: File) =
    <job_chain name={TestJobChainPath.name}>
      <file_order_source directory={directory.getPath}/>
      <job_chain_node state="100" job="/test"/>
      <file_order_sink state={SinkState.string} remove="yes"/>
    </job_chain>
}
