package com.sos.scheduler.engine.kernel.time

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import org.joda.time.DateTimeZone.UTC
import org.joda.time.DateTimeConstants._
import org.joda.time.{LocalDateTime, DateTimeZone}

@ForCpp
object TimeZones {
  //FIXME Was tun bei einer Exception, einer unbekannten Zeitzone? Besser nur vorher bekannte, numerierte nehmen

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

  private def zone(name: String) = name match {
    case "" => DateTimeZone.getDefault
    case _ => DateTimeZone.forID(name)
  }
}
