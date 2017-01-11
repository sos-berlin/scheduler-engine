package com.sos.scheduler.engine.tests.jira.js1681

import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1681.JS1681IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * JS-1681 Global parameters from scheduler.xml should be available if a pre-processing script is defined.
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1681IT extends FreeSpec with ScalaSchedulerTest {

  "JS-1681" in {
    val results = JobPaths map { jobPath ⇒ startJob(jobPath).result } await TestTimeout
    for ((jobPath, result) ← JobPaths zip results) {
      assert(result.logString contains s"SCHEDULER_PARAM_JOB=${jobPath.name}", s", in $jobPath")
      assert(result.logString contains "SCHEDULER_PARAM_GLOBAL=GLOBAL VALUE", s", in $jobPath")
    }
  }
}

private object JS1681IT {
  private val JobPaths = List(JobPath("/simple"), JobPath("/monitor"))
}
