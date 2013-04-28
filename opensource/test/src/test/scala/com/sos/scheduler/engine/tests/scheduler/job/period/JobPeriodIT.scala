package com.sos.scheduler.engine.tests.scheduler.job.period

import JobPeriodIT._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.TaskStartedEvent
import com.sos.scheduler.engine.eventbus.EventHandler
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import java.lang.Thread.sleep
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import scala.collection.mutable

@RunWith(classOf[JUnitRunner])
class JobPeriodIT extends ScalaSchedulerTest {

  private val counts = mutable.HashMap[JobPath, Int](apiJobPath -> 0, shellJobPath -> 0)
  private val n = 5
  private val interval = 2.s

  for (jobPath <- Seq(apiJobPath, shellJobPath)) {
    test(s"Task $jobPath should start about $n times") {
      unstopJob(jobPath)
      sleep(((n+1) * interval).getMillis + 900)
      stopJob(jobPath)
      counts(jobPath) should  be (n plusOrMinus 2)
    }
  }

  @EventHandler def handle(e: TaskStartedEvent) {
    counts(e.jobPath) += 1
  }

  private def stopJob(jobPath: JobPath) {
    scheduler executeXml <modify_job job={jobPath.string} cmd="stop"/>
  }

  private def unstopJob(jobPath: JobPath) {
    scheduler executeXml <commands>
      <modify_job job={jobPath.string} cmd="enable"/>
      <modify_job job={jobPath.string} cmd="unstop"/>
    </commands>
  }
}

private object JobPeriodIT {
  val apiJobPath = JobPath.of("/test-api")
  val shellJobPath = JobPath.of("/test-shell")
}
