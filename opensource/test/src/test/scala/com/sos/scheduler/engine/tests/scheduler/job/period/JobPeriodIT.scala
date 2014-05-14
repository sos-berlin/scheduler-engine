package com.sos.scheduler.engine.tests.scheduler.job.period

import JobPeriodIT._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.job.{JobPath, TaskStartedEvent}
import com.sos.scheduler.engine.eventbus.EventHandler
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.joda.time.Instant.now
import org.joda.time.{Instant, Duration}
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.{immutable, mutable}

@RunWith(classOf[JUnitRunner])
final class JobPeriodIT extends FunSuite with ScalaSchedulerTest {

  private val counts = mutable.HashMap[JobPath, Int]() ++ (jobConfigs map { _.path -> 0 })
  private val n = 3

  for (j <- jobConfigs) {
    val duration = n * j.interval + 500.ms
    test(s"Task ${j.path} should start about $n times in ${duration.getMillis}ms") {
      sleep(durationUntilNextInterval(j.interval, (-900).ms))
      scheduler executeXml j.xmlElem
      sleep(duration)
      counts(j.path) should be (n +- 1)
      stopJob(j.path)
    }
  }

  @EventHandler def handle(e: TaskStartedEvent) {
    counts(e.jobPath) += 1
  }

  private def stopJob(jobPath: JobPath) {
    scheduler executeXml <modify_job job={jobPath.string} cmd="stop"/>
  }
}

private object JobPeriodIT {
  private def durationUntilNextInterval(interval: Duration, shift: Duration) = {
    val i = interval.getMillis
    val nw = now() - shift
    new Instant(nw.getMillis / i * i + i) - nw
  }

  private trait JobConfig {
    val path: JobPath
    val interval: Duration
    val xmlElem: xml.Elem
  }

  private val jobConfigs = immutable.Seq(
    new JobConfig {
      val path = JobPath("/test-shell")
      val interval = 1.s
      val xmlElem =
        <job name={path.name}>
          <script language="shell">exit 0</script>
          <run_time>
            <period absolute_repeat={interval.getStandardSeconds.toString}/>
          </run_time>
        </job>
    },
    new JobConfig {
      val path = JobPath("/test-api")
      val interval = 4.s  // Eine API-Task braucht schon 1s zum Start (je nach Rechner) 2013-05-15
      val xmlElem =
        <job name={path.name}>
          <script java_class="com.sos.scheduler.engine.test.jobs.SingleStepJob"/>
          <run_time>
            <period absolute_repeat={interval.getStandardSeconds.toString}/>
          </run_time>
        </job>
    })
}
