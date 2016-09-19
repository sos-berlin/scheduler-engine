package com.sos.scheduler.engine.tests.jira.js1470

import com.sos.scheduler.engine.data.job.{JobPath, TaskStarted}
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1470 sos.spooler.Spooler.start(jobPath) did not work.
 *
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1470IT extends FreeSpec with ScalaSchedulerTest {

  "sos.spooler.Spooler.start(jobPath)" in {
    eventBus.awaitingWhen[TaskStarted.type](_.key.jobPath == JobPath("/test-b")) {
      startJob(JobPath("/test-a"))
    }
  }
}
