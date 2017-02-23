package com.sos.scheduler.engine.tests.jira.js779

import com.sos.jobscheduler.common.scalautil.Futures.implicits.SuccessFuture
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * JS-729 If task is terminated due to timeout or a kill task command, then that should be written in the error element of the history entry.
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS779IT extends FreeSpec with ScalaSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(testClass, ignoreError = { _ ⇒ true })

  "kill" in {
    val run = startJob(JobPath("/test"))
    run.started await 10.s
    scheduler executeXml <kill_task job={run.jobPath.string} id={run.taskId.string} immediately="true"/>
    run.result await 10.s
    checkFor(run, "SCHEDULER-728")  // "Terminating task after kill_task"
  }

  "timeout" in {
    val run = startJob(JobPath("/test-timeout"))
    run.result await 10.s
    checkFor(run, "SCHEDULER-272") // "Terminating task after reaching deadline"
  }

  private def checkFor(run: TaskRun, errorCode: String): Unit = {
    val error = (scheduler executeXml <show_job job={run.jobPath.string}/>).answer \ "job" \ "ERROR"
    assert((error \ "@text").toString startsWith s"$errorCode ")  // "Terminating task after reaching deadline"
    assert((error \ "@code").toString == errorCode)  // "Task logs error"

    val historyEntry = (scheduler executeXml <show_history job={run.jobPath.string} id={run.taskId.string}/>).answer \ "history" \ "history.entry"
    assert((historyEntry \ "@error_text").toString startsWith s"$errorCode ")
    assert((historyEntry \ "@error_code").toString == errorCode)
  }
}
