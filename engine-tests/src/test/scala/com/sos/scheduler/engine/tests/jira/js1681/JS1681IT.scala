package com.sos.scheduler.engine.tests.jira.js1681

import com.sos.jobscheduler.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1681.JS1681IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * JS-1681 Global parameters from scheduler.xml should be available if a pre-processing script is defined.
  * JS-1681 Shell jobs should have an environment with an unchanged LD_LIBRARY_PATH which is modified by the startscript so that the JobScheduler finds some own libraries.
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1681IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-env=LD_LIBRARY_PATH=$TestLibraryPath")  // The test framework set a first -env=LD_LIBRARY_PATH. Both are handle by spooler.cxx.
  )

  "JS-1681, JS-1681" in {
    val results = JobPaths map { jobPath ⇒ startJob(jobPath).result } await TestTimeout
    for ((jobPath, result) ← JobPaths zip results) {
      // JS-1681
      assert(result.logString contains s"SCHEDULER_PARAM_JOB=JOB-VALUE", s", in $jobPath")
      assert(result.logString contains "SCHEDULER_PARAM_GLOBAL=GLOBAL-VALUE", s", in $jobPath")

      // JS-1681
      if (!jobPath.string.contains("agent")) {
        assert(result.logString contains s"LD_LIBRARY_PATH=$TestLibraryPath", s", in $jobPath")
      }
      if (jobPath == MonitorJobPath) {
        assert(result.logString contains s"LD_LIBRARY_PATH(monitor)=Some($TestLibraryPath)", s", in $jobPath")
      }
    }
  }
}

private object JS1681IT {
  private val TestLibraryPath = "TEST-LIBRARY-PATH"
  private val MonitorJobPath = JobPath("/monitor")
  private val JobPaths = List(JobPath("/simple"), MonitorJobPath, JobPath("/agent-simple"), JobPath("/agent-monitor"))
}
