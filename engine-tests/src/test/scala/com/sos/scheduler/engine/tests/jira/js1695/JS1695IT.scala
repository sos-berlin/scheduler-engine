package com.sos.scheduler.engine.tests.jira.js1695

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
final class JS1695IT extends FreeSpec with ScalaSchedulerTest {

  "API Spooler.create_subprocess in Java" in {
    runJob(JobPath("/java"))
  }
}
