package com.sos.scheduler.engine.tests.jira.js1723

import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1723IT extends FreeSpec with ScalaSchedulerTest {

  if (sys.props contains "JS-1723")  // Manual test
    "JS-1723" in {
      runJob(JobPath("/test"))
    }
}
