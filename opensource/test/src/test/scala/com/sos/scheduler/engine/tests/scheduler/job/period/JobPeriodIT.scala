package com.sos.scheduler.engine.tests.scheduler.job.period

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.TaskStartedEvent
import com.sos.scheduler.engine.eventbus.EventHandler
import com.sos.scheduler.engine.kernel.job.JobSubsystem
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

@RunWith(classOf[JUnitRunner])
class JobPeriodIT extends ScalaSchedulerTest {

  import JobPeriodIT._

  private var count = 0
  private val n = 3

  test(s"Task should start about $n times") {
    Thread.sleep(n*1000)
    count should (be >= (n-1) and be <= (n+1))
  }

  @EventHandler def handle(e: TaskStartedEvent) {
    if (e.jobPath == jobPath)
      count += 1
  }
}

private object JobPeriodIT {
  val jobPath = JobPath.of("/a")
}
