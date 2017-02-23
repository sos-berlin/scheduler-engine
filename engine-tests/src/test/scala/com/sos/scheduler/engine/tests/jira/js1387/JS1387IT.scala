package com.sos.scheduler.engine.tests.jira.js1387

import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.time.SchedulerDateTime.{formatLocally, formatUtc}
import com.sos.scheduler.engine.kernel.Scheduler
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1387.JS1387IT._
import java.time.Instant.now
import java.time.temporal.ChronoUnit.SECONDS
import java.time.{Instant, LocalDateTime, LocalTime, Period, ZonedDateTime}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
  * JS-1387, JS-1686, JS-1687, JS-1691 &lt;show_calendar>.
  *
  * @author Joacim Zschimmer
  */
@RunWith(classOf[JUnitRunner])
final class JS1387IT extends FreeSpec with ScalaSchedulerTest {
  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List("-distributed-orders"))

  s"Command show_calendar" in {
    for (o ← TimedObjects) scheduler executeXml o.toCommand
    val answer = (scheduler executeXml <show_calendar what="orders" before="2030-12-31T12:00:00Z" limit="10"/>).answer
    val entries = (answer \ "calendar" \ "_") collect { case e: xml.Elem ⇒ e }
    val expected = TimedObjects flatMap { _.toExpectedCalendarEntries }
    logger.info(entries.mkString("\n", "\n", ""))
    if (entries.toSet != expected.toSet) logger.info("BUT EXPECTED WAS: " + expected.mkString("\n", "\n", ""))
    assert(entries.size == expected.size)
    assert(entries.toSet == expected.toSet)
  }
}

private[js1387] object JS1387IT {
  private val NonDistributedJobChainPath = JobChainPath("/test-non-distributed")
  private val DistributedJobChainPath = JobChainPath("/test-distributed")
  private val SingleStartJobPath = JobPath("/SingleStart")
  private val NoOrderJobPath = JobPath("/NoOrder")
  private val logger = Logger(getClass)

  private val TimedObjects = {
    val t = now truncatedTo SECONDS plus Period.ofDays(1)
    Vector(
      AtTimedOrder          (NonDistributedJobChainPath, t),
      RuntimeAtTimedOrder   (NonDistributedJobChainPath, t + 1.s),
      SingleStartTimedOrder (NonDistributedJobChainPath, LocalTime.of(1, 1, 1)),
      RepeatTimedOrder      (NonDistributedJobChainPath, LocalTime.of(2, 2, 2)),
      AtTimedOrder          (DistributedJobChainPath, t + 2.s),
      RuntimeAtTimedOrder   (DistributedJobChainPath, t + 3.s),
      SingleStartTimedOrder (DistributedJobChainPath, LocalTime.of(3, 3, 3)),
      RepeatTimedOrder      (DistributedJobChainPath, LocalTime.of(4, 4, 4)),
      SingleStartJob        (SingleStartJobPath, LocalTime.of(7, 7, 7)),
      AtJob                 (NoOrderJobPath, t + 4.s))
  }

  private trait Timed {
    def toCommand: xml.Elem
    def toExpectedCalendarEntries: List[xml.Elem]
  }

  private trait TimedOrder extends Timed {
    val jobChainPath: JobChainPath
    def orderKey = jobChainPath orderKey getClass.getSimpleName
  }

  private final case class AtTimedOrder(jobChainPath: JobChainPath, at: Instant) extends TimedOrder {
    def toCommand =
      <order job_chain={orderKey.jobChainPath.string}
             id={orderKey.id.string}
             at={formatLocally(Scheduler.DefaultZoneId, at)}/>

    def toExpectedCalendarEntries = List(
      <at job_chain={orderKey.jobChainPath.string}
          order={orderKey.id.string}
          at={formatUtc(at)}/>)
  }

  private final case class RuntimeAtTimedOrder(jobChainPath: JobChainPath, at: Instant) extends TimedOrder {
    def toCommand =
      <order job_chain={orderKey.jobChainPath.string} id={orderKey.id.string}>
        <run_time>
          <at at={formatLocally(Scheduler.DefaultZoneId, at)}/>
        </run_time>
      </order>

    def toExpectedCalendarEntries = List(
      <period job_chain={orderKey.jobChainPath.string}
          order={orderKey.id.string}
          single_start={formatUtc(at)}/>,
      <at job_chain={orderKey.jobChainPath.string}
          order={orderKey.id.string}
          at={formatUtc(at)}/>)
  }

  private final case class SingleStartTimedOrder(jobChainPath: JobChainPath, singleStart: LocalTime) extends TimedOrder {
    private val tomorrow = LocalDateTime.ofInstant(now, Scheduler.DefaultZoneId).toLocalDate plusDays 1
    private val at = ZonedDateTime.of(tomorrow, singleStart, Scheduler.DefaultZoneId).toInstant

    def toCommand =
      <order job_chain={orderKey.jobChainPath.string} id={orderKey.id.string}>
        <run_time>
          <date date={tomorrow.toString}>
            <period single_start={singleStart.toString}/>
          </date>
        </run_time>
      </order>

    def toExpectedCalendarEntries = List(
      <period job_chain={orderKey.jobChainPath.string}
          order={orderKey.id.string}
          single_start={formatUtc(at)}/>,
      <at job_chain={orderKey.jobChainPath.string}
          order={orderKey.id.string}
          at={formatUtc(at)}/>)
  }

  private final case class RepeatTimedOrder(jobChainPath: JobChainPath, localTime: LocalTime) extends TimedOrder {
    private val tomorrow = LocalDateTime.ofInstant(now, Scheduler.DefaultZoneId).toLocalDate plusDays 1
    private val at = ZonedDateTime.of(tomorrow, localTime, Scheduler.DefaultZoneId).toInstant
    private val end = ZonedDateTime.of(tomorrow plusDays 1, LocalTime.of(0, 0), Scheduler.DefaultZoneId).toInstant

    def toCommand =
      <order job_chain={orderKey.jobChainPath.string} id={orderKey.id.string}>
        <run_time>
          <date date={tomorrow.toString}>
            <period begin={localTime.toString} absolute_repeat="01:00"/>
          </date>
        </run_time>
      </order>

    def toExpectedCalendarEntries = List(
      <period job_chain={orderKey.jobChainPath.string}
          order={orderKey.id.string}
          begin={formatUtc(at)}
          end={formatUtc(end)}
          absolute_repeat="3600"/>,
      <at job_chain={orderKey.jobChainPath.string}
          order={orderKey.id.string}
          at={formatUtc(at)}/>)
  }

  private case class AtJob(jobPath: JobPath, at: Instant) extends Timed {
    def toCommand =
      <start_job job={jobPath.string}  at={formatLocally(Scheduler.DefaultZoneId, at)}/>

    def toExpectedCalendarEntries = List(
      <at job={jobPath.withoutStartingSlash} at={formatUtc(at)} task={TaskId.First.string}/>)
  }

  private case class SingleStartJob(jobPath: JobPath, singleStart: LocalTime) extends Timed {
    private val tomorrow = LocalDateTime.ofInstant(now, Scheduler.DefaultZoneId).toLocalDate plusDays 1
    private val at = ZonedDateTime.of(tomorrow, singleStart, Scheduler.DefaultZoneId).toInstant

    def toCommand =
      <job name={jobPath.name}>
        <script language="shell">exit</script>
        <run_time>
          <date date={tomorrow.toString}>
            <period single_start={singleStart.toString}/>
          </date>
        </run_time>
      </job>

    def toExpectedCalendarEntries = List(
      <period job={jobPath.string} single_start={formatUtc(at)}/>)
  }
}
