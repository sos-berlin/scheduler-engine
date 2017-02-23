package com.sos.scheduler.engine.kernel.time

import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import org.joda.time.DateTimeConstants._
import org.joda.time.DateTimeZone.UTC
import org.joda.time.format.DateTimeFormat
import org.joda.time.{DateTimeZone, LocalDateTime}

@ForCpp
object TimeZones {
  private val localTimeZone = DateTimeZone.getDefault
  private val logger = Logger(getClass)

  /**
   * Stores local time zone before someone can change it.
   */
  def initialize(): Unit = {
    logger.debug(s"Our time zone is '$localTimeZone'")
  }

  private val timeZoneFormatter = Map(
    false -> DateTimeFormat.forPattern("yyyy-MM-dd HH:mm:ssZ"),
    true -> DateTimeFormat.forPattern("yyyy-MM-dd HH:mm:ss.SSSZ"))

  @ForCpp def localToUtc(timeZoneName: String, localMillis: Long): Long = {
    val z = zone(timeZoneName)
    val offset = z.getOffsetFromLocal(localMillis)
    localMillis - offset match {
      case result if z.getOffset(result) == offset => result
      case _ => z.nextTransition(localMillis - MILLIS_PER_DAY)   // Übersprungene Stunde => Anfang des Zeitwechsels
    }
  }

  @ForCpp def utcToLocal(timeZoneName: String, millis: Long): Long =
    new LocalDateTime(millis, zone(timeZoneName)).toDateTime(UTC).getMillis

  @ForCpp def toString(timeZoneName: String, withMillis: Boolean, utcTime: Long): String = {
    val r = timeZoneFormatter(withMillis) withZone zone(timeZoneName) print utcTime
    if (r endsWith "+0000") r.substring(0, r.length - 5) + 'Z' else r
  }

  private def zone(name: String) = name match {
    case "" => localTimeZone
    case _ => DateTimeZone.forID(name)
  }
}
