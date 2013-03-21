package com.sos.scheduler.engine.tests.scheduler.job.period

import JobPeriodIT._
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.TaskStartedEvent
import com.sos.scheduler.engine.eventbus.EventHandler
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._

@RunWith(classOf[JUnitRunner])
class JobPeriodIT extends ScalaSchedulerTest {

  override val logCategories = "all sleep scheduler.loop scheduler.wait"

  private var count = 0
  private val n = 4

  test(s"Task should start about $n times") {
    Thread.sleep(n*1000)
    count should be (n plusOrMinus 2)
  }

  @EventHandler def handle(e: TaskStartedEvent) {
    if (e.jobPath == jobPath)
      count += 1
  }
}

private object JobPeriodIT {
  val jobPath = JobPath.of("/a")
}
