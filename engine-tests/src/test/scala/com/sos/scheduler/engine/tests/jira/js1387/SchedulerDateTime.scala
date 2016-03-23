package com.sos.scheduler.engine.tests.jira.js1387

import com.sos.scheduler.engine.kernel.Scheduler
import java.time.format.DateTimeFormatterBuilder
import java.time.temporal.ChronoField._
import java.time.{Instant, ZoneId, ZonedDateTime}

/**
  * @author Joacim Zschimmer
  */
object SchedulerDateTime {
  private def newDateFormatBuilder() =
    new DateTimeFormatterBuilder()
    .appendValue(YEAR, 4).appendLiteral('-').appendValue(MONTH_OF_YEAR, 2).appendLiteral('-').appendValue(DAY_OF_MONTH, 2)
    .appendLiteral('T')
    .appendValue(HOUR_OF_DAY, 2).appendLiteral(':').appendValue(MINUTE_OF_HOUR, 2).appendLiteral(':').appendValue(SECOND_OF_MINUTE, 2)

  private val utc = ZoneId.of("UTC")
  private val LocalFormat = newDateFormatBuilder().toFormatter.withZone(ZoneId.of(Scheduler.defaultTimezoneId))

  def formatLocally(instant: Instant) = LocalFormat.format(ZonedDateTime.ofInstant(instant, utc))

  val formatUtc = newDateFormatBuilder().appendLiteral('Z').toFormatter.withZone(ZoneId.of("UTC")).format _
}
