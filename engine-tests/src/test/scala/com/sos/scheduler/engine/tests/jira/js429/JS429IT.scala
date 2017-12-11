package com.sos.scheduler.engine.tests.jira.js429

import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import java.time.format.DateTimeFormatter.ISO_OFFSET_DATE_TIME
import java.time.format.{DateTimeFormatter, FormatStyle}
import java.time.{LocalDateTime, ZoneOffset, ZonedDateTime}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/** JS-429 Environment variables for date and time with jobs on Agents.
  */
@RunWith(classOf[JUnitRunner])
final class JS429IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  private var log = ""
  private var local = LocalDateTime.ofEpochSecond(0, 0, ZoneOffset.ofHours(0))  // In case of test failure
  private val localFormatter = DateTimeFormatter.ofLocalizedDateTime(FormatStyle.MEDIUM)

  "Job" in {
    log = runJob(JobPath("/test")).logString
  }

  "SCHEDULER_TASKSTART_DATE_ISO" in {
    val dateIso = value("SCHEDULER_TASKSTART_DATE_ISO")
    val zonedDateTime = ZonedDateTime.parse(dateIso)
    local = zonedDateTime.toLocalDateTime
    assert(dateIso startsWith ISO_OFFSET_DATE_TIME.format(zonedDateTime).take(11))   // "yyyy-mm-ddT...", may fail around midnight
    assert(!dateIso.contains(".")) // Only second-precision
  }

  "SCHEDULER_TASKSTART_DATE" in {
    assert(value("SCHEDULER_TASKSTART_DATE") == localFormatter.format(local))
  }

  "SCHEDULER_TASKSTART_YEAR" in {
    assert(value("SCHEDULER_TASKSTART_YEAR") == f"${local.getYear}%04d")
  }

  "SCHEDULER_TASKSTART_MONTH" in {
    assert(value("SCHEDULER_TASKSTART_MONTH") == f"${local.getMonthValue}%02d")
  }

  "SCHEDULER_TASKSTART_DAY" in {
    assert(value("SCHEDULER_TASKSTART_DAY") == f"${local.getDayOfMonth}%02d")
  }

  "SCHEDULER_TASKSTART_HOUR" in {
    assert(value("SCHEDULER_TASKSTART_HOUR") == f"${local.getHour}%02d")
  }

  "SCHEDULER_TASKSTART_MINUTE" in {
    assert(value("SCHEDULER_TASKSTART_MINUTE") == f"${local.getMinute}%02d")
  }

  "SCHEDULER_TASKSTART_SECOND" in {
    assert(value("SCHEDULER_TASKSTART_SECOND") == f"${local.getSecond}%02d")
  }

  private def value(key: String) = s"$key=/(.*)/".r findFirstMatchIn log map (_.group(1)) getOrElse
    fail(s"Missing $key=/.../ in job output:\n$log")
}
