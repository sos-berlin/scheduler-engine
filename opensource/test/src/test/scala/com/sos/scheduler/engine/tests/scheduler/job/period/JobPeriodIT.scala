package com.sos.scheduler.engine.tests.scheduler.job.period

import JobPeriodIT._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.TaskStartedEvent
import com.sos.scheduler.engine.eventbus.EventHandler
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import scala.collection.mutable

@RunWith(classOf[JUnitRunner])
class JobPeriodIT extends ScalaSchedulerTest {

  private var counts = mutable.HashMap[JobPath, Int](apiJobPath -> 0, shellJobPath -> 0)
  private val n = 5
  private val interval = 2.s

  for (jobPath <- Seq(apiJobPath, shellJobPath)) {
    test(s"Task $jobPath should start about $n times") {
      scheduler executeXml <commands>
        <modify_job job={jobPath.string} cmd="enable"/>
        <modify_job job={jobPath.string} cmd="unstop"/>
      </commands>
      Thread.sleep(((n+1) * interval).getMillis + 900)
      scheduler executeXml <modify_job job={jobPath.string} cmd="stop"/>
      counts(jobPath) should be (n plusOrMinus 2)
    }
  }

  @EventHandler def handle(e: TaskStartedEvent) {
    counts(e.jobPath) += 1
  }
}

private object JobPeriodIT {
  val apiJobPath = JobPath.of("/test-api")
  val shellJobPath = JobPath.of("/test-shell")
}
