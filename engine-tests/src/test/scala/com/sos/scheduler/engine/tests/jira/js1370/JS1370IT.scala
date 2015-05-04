package com.sos.scheduler.engine.tests.jira.js1370

import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1370 Under Unix, JobScheduler API fails when being passed a double value.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1370IT extends FreeSpec with ScalaSchedulerTest {

  "JS1370IT" in {
    runJobAndWaitForEnd(JobPath("/test"))
  }
}
