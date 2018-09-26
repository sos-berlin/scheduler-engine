package com.sos.scheduler.engine.taskserver.modules.shell

import java.time.format.{DateTimeFormatter, FormatStyle}
import java.time.{Instant, ZoneId, ZonedDateTime}
import java.util.Locale
import scala.io

object TaskVariables {
  private val LocalDateTimeFormatter = DateTimeFormatter.ofLocalizedDateTime(FormatStyle.MEDIUM) withLocale Locale.getDefault

  /** JS-429 */
  private[shell] def taskStartVariables(instant: Instant): Map[String, String] = {
    val secondInstant = Instant.ofEpochSecond(instant.toEpochMilli / 1000)
    val zonedDateTime = ZonedDateTime.ofInstant(secondInstant, ZoneId.systemDefault)
    val local = zonedDateTime.toLocalDateTime
    Map(
      "SCHEDULER_TASKSTART_DATE_ISO" → DateTimeFormatter.ISO_OFFSET_DATE_TIME.format(zonedDateTime),
      "SCHEDULER_TASKSTART_DATE"     → LocalDateTimeFormatter.format(local),
      "SCHEDULER_TASKSTART_YEAR"     → f"${local.getYear      }%04d",
      "SCHEDULER_TASKSTART_MONTH"    → f"${local.getMonthValue}%02d",
      "SCHEDULER_TASKSTART_DAY"      → f"${local.getDayOfMonth}%02d",
      "SCHEDULER_TASKSTART_HOUR"     → f"${local.getHour      }%02d",
      "SCHEDULER_TASKSTART_MINUTE"   → f"${local.getMinute    }%02d",
      "SCHEDULER_TASKSTART_SECOND"   → f"${local.getSecond    }%02d")
  }
}
