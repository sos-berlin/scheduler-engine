package com.sos.scheduler.engine.tests.jira.js1635

import com.sos.jobscheduler.common.scalautil.Futures.implicits._
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.log.WarningLogged
import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import java.time.Instant.now
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * JS-1635 warn_if_longer_than should work for shell jobs.
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1635IT extends FreeSpec with ScalaSchedulerTest {

  "warn_if_longer_than" in {
    val jobPaths = List(JobPath("/shell"), JobPath("/shell-with-monitor"), JobPath("/api"))
    var runs: List[TaskRun] = null
    val t = now
    eventBus.awaitingWhen[WarningLogged](_.event.codeOption contains MessageCode("SCHEDULER-712")) {
      runs = for (jobPath ← jobPaths) yield startJob(jobPath)
    }
    val duration = now - t
    assert(duration >= 5.s && duration <= 9.s)
    assert(runs forall { o ⇒ !o.ended.isCompleted })
    val results = runs map { _.result } await TestTimeout
    assert(results forall { o ⇒ o.duration >= 10.s })
  }
}
