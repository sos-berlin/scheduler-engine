package com.sos.scheduler.engine.tests.scheduler.job.wake_when_not_in_period

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.job.TaskStartedEvent
import com.sos.scheduler.engine.eventbus.EventHandler
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import java.lang.Thread.sleep
import org.joda.time.DateTimeConstants.MILLIS_PER_DAY
import org.joda.time._
import org.joda.time.format.DateTimeFormat
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import scala.collection.mutable

/** JS-948 */
@RunWith(classOf[JUnitRunner])
class WakeWhenInPeriodIT extends ScalaSchedulerTest {

  import WakeWhenInPeriodIT._

  private val startTimes = mutable.Buffer[LocalTime]()

  test("wake_when_in_period") {
    val now = new LocalTime()
    val minimumTimeUntilMidnight = 10000
    if (now isAfter now.withMillisOfDay(MILLIS_PER_DAY - minimumTimeUntilMidnight))
      sleep(minimumTimeUntilMidnight + 1000)

    val t = new LocalTime(now) plusMillis 1999 withMillisOfSecond 0
    val a = Period(t plusSeconds 2, t plusSeconds 4)
    val b = Period(t plusSeconds 6, t plusSeconds 8)
    scheduler executeXml jobElem(List(a, b))

    scheduler executeXml <modify_job job={jobPath.string} cmd="wake_when_in_period"/>   // Vor der Periode: unwirksam
    sleepUntil(a.begin plusMillis 100)
    scheduler executeXml <modify_job job={jobPath.string} cmd="wake_when_in_period"/>   // In der Periode: wirksam
    sleepUntil(a.end plusMillis 100)
    scheduler executeXml <modify_job job={jobPath.string} cmd="wake_when_in_period"/>   // Nach der Periode: unwirksam
    sleepUntil(b.begin plusMillis 500)

    startTimes should have size(1)
    assert(a contains startTimes(0))
  }

  @EventHandler def handle(e: TaskStartedEvent) {
    if (e.jobPath == jobPath)
      startTimes += new LocalTime
  }
}

private object WakeWhenInPeriodIT {
  val jobPath = JobPath.of("/a")
  val hhmmssFormat = DateTimeFormat.forPattern("HH:mm:ss")

  def jobElem(periods: Iterable[Period]) =
    <job name={jobPath.getName}>
      <script java_class="com.sos.scheduler.engine.test.jobs.SingleStepJob"/>
      <run_time>{ periods map { o => <period begin={hhmmssFormat.print(o.begin)} end={hhmmssFormat.print(o.end)}/>} }</run_time>
    </job>

  case class Period(begin: LocalTime, end: LocalTime) {
    def contains(t: LocalTime) = !(t isBefore begin) && (t isBefore end)
  }

  def sleepUntil(t: LocalTime) {
    sleep(t.getMillisOfDay - (new LocalTime).getMillisOfDay)
  }
}
