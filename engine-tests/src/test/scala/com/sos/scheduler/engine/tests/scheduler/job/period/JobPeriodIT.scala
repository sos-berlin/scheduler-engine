package com.sos.scheduler.engine.tests.scheduler.job.period

import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.jobscheduler.data.event.KeyedEvent
import com.sos.scheduler.engine.data.job.{JobPath, TaskKey, TaskStarted}
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.scheduler.job.period.JobPeriodIT._
import org.joda.time.Instant.now
import org.joda.time.{DateTimeZone, Duration, Instant}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable

@RunWith(classOf[JUnitRunner])
final class JobPeriodIT extends FreeSpec with ScalaSchedulerTest {

  private val counts = mutable.HashMap[JobPath, Int]() ++ (JobConfigs map { _.path → 0 })
  private val n = 3

  eventBus.on[TaskStarted.type] {
    case KeyedEvent(TaskKey(jobPath, _), _) ⇒
      counts(jobPath) += 1
  }

  for (j ← JobConfigs) {
    val duration = n * j.interval + 500.ms
    s"Task ${j.path} should start about $n times in ${duration.getMillis}ms" in {
      sleep(durationUntilNextInterval(j.interval, (-900).ms))
      scheduler executeXml j.xmlElem
      sleep(duration)
      withClue(s"At ${now toDateTime DateTimeZone.getDefault}: ") {
        counts(j.path) should be (n +- 1)
      }
      stopJob(j.path)
    }
  }

  private def stopJob(jobPath: JobPath) =
    scheduler executeXml <modify_job job={jobPath.string} cmd="stop"/>
}

private object JobPeriodIT {
  private def durationUntilNextInterval(interval: Duration, shift: Duration) = {
    val i = interval.getMillis
    val nw = now - shift
    new Instant(nw.getMillis / i * i + i) - nw
  }

  private trait JobConfig {
    val path: JobPath
    val interval: Duration
    val xmlElem: xml.Elem
  }

  private val JobConfigs = List(
    new JobConfig {
      val path = JobPath("/test-shell")
      val interval = 2.s
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
      val interval = 5.s  // Eine API-Task braucht schon 1s zum Start (je nach Rechner) 2013-05-15
      val xmlElem =
        <job name={path.name}>
          <script java_class="com.sos.scheduler.engine.test.jobs.SingleStepJob"/>
          <run_time>
            <period absolute_repeat={interval.getStandardSeconds.toString}/>
          </run_time>
        </job>
    })
}
