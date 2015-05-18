package com.sos.scheduler.engine.tests.scheduler.comapi.javascript

import com.sos.scheduler.engine.agent.Agent
import com.sos.scheduler.engine.agent.configuration.AgentConfiguration
import com.sos.scheduler.engine.agent.test.AgentTest
import com.sos.scheduler.engine.agent.test.AgentTest._
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.Futures._
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder._
import com.sos.scheduler.engine.data.event.Event
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.log.InfoLogEvent
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.scheduler.comapi.javascript.SchedulerAPIJavascriptIT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

import scala.collection.immutable
import scala.concurrent.Promise
import scala.concurrent.duration._

/**
 * @author Andreas Liebert
 */
@RunWith(classOf[JUnitRunner])
final class SchedulerAPIJavascriptIT extends FreeSpec with ScalaSchedulerTest with AgentTest{

  private val finishedOrderParametersPromise = Promise[Map[String, String]]()
  private val eventsPromise = Promise[immutable.Seq[Event]]()
  private lazy val taskLogLines = eventsPromise.successValue collect { case e: InfoLogEvent ⇒ e.message }

  "javascript job" - {
    for ((name, jobPath) ← List("Without Agent" → JavascriptJobPath, "With Agent" -> JavascriptJobPath.asAgent)) {
      name in {
        val run = runJobFuture(jobPath)
        val taskResult: TaskResult = awaitSuccess(run.result)
        taskResult.logString should include("Hello world")
      }
    }
  }

  "shell job with javascript monitor" - {
    for ((name, jobPath) ← List("Without Agent" → JavascriptMonitorJobPath, "With Agent" -> JavascriptMonitorJobPath.asAgent)) {
      name in {
        val run = runJobFuture(jobPath)
        val taskResult: TaskResult = awaitSuccess(run.result)
        taskResult.logString should include("Hello world")
        taskResult.logString should include("##this is spooler_task_before##")
        taskResult.logString should include("##this is spooler_process_before##")
        taskResult.logString should include("##this is spooler_process_after##")
      }
    }
  }

}

object SchedulerAPIJavascriptIT {
  private val JavascriptJobPath = JobPath("/javascript")
  private val JavascriptMonitorJobPath = JobPath("/javascript_monitor")
  private val ProcessClassName = "test-agent"
}




