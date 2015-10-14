package com.sos.scheduler.engine.tests.jira.js1311

import JS1311IT._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.FreeSpec
import org.scalatest.Matchers._

/**
 * @author Joacim Zschimmer
 */
// This is a manual test. @RunWith(classOf[JUnitRunner])
final class JS1311IT extends FreeSpec with ScalaSchedulerTest {

  "holidays with run_time" in {
    runJobFuture(JobPath("/test"))
    sleep(300.s)
  }
}

private object JS1311IT {
  private val logger = Logger(getClass)
}
