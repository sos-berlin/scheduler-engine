package com.sos.scheduler.engine.tests.jira.js946

import JS946IT._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS946IT extends FreeSpec with ScalaSchedulerTest {

  protected override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    ignoreError = Set(
      "SCHEDULER-280",  // "Process terminated with exit code 1"
      "Z-REMOTE-118"))  // "Separate process pid=0: No response from new process within 60s"

  "Task does not start within a minute" in {
    runJobAndWaitForEnd(unreachableRemoteJobPath, 70.s)
  }
}

private object JS946IT {
  private val unreachableRemoteJobPath = JobPath("/test-unreachable-remote")
}