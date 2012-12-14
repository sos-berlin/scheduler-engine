package com.sos.scheduler.engine.kernel.time

import org.joda.time.DateTimeConstants._
import org.joda.time.DateTimeZone.UTC
import org.joda.time.{DateTimeZone, LocalDateTime, DateTime}
import org.scalatest.FunSuite
import org.scalatest.matchers.ShouldMatchers._
import scala.collection.immutable

final class TimeZonesTest extends FunSuite {

  import TimeZonesTest._

  private val springHelsinkiToUtc = immutable.Seq(
      Helsinki(2012,  3, 25, 2, 30) -> Utc(2012,  3, 25,  0, 30) -> 120,   // Noch Winterzeit
      Helsinki(2012,  3, 25, 3,  0) -> Utc(2012,  3, 25,  1,  0) -> 120,   // Beginn der Sommerzeit, Sprung von 3 auf 4 Uhr
      Helsinki(2012,  3, 25, 3, 30) -> Utc(2012,  3, 25,  1,  0) -> 150,   // Diese lokale Zeit gibt es nicht, die Stunde fehlt
      Helsinki(2012,  3, 25, 4,  0) -> Utc(2012,  3, 25,  1,  0) -> 180,
      Helsinki(2012,  3, 25, 4, 30) -> Utc(2012,  3, 25,  1, 30) -> 180)
  private val fallHelsinkiToUtc = immutable.Seq(
      Helsinki(2012, 10, 28, 2, 30) -> Utc(2012, 10, 27, 23, 30) -> 180,   // +3h Noch Sommerzeit
      Helsinki(2012, 10, 28, 3,  0) -> Utc(2012, 10, 28,  0,  0) -> 180,   // +3h 2 bis 3 Uhr lokaler Zeit ist doppelt
      Helsinki(2012, 10, 28, 3, 30) -> Utc(2012, 10, 28,  0, 30) -> 180,   // +3h
      Helsinki(2012, 10, 28, 4,  0) -> Utc(2012, 10, 28,  2,  0) -> 120,   // +2h Ende der Sommerzeit, Sprung von 4 auf 3 Uhr
      Helsinki(2012, 10, 28, 4, 30) -> Utc(2012, 10, 28,  2, 30) -> 120)   // +2h Winterzeit

  for (((local, utc), offsetMinutes) <- springHelsinkiToUtc ++ fallHelsinkiToUtc) {
    test(local +" should yield "+ utc) {
      local.millis - utc.millis should equal (offsetMinutes *MILLIS_PER_MINUTE)
      val diff = TimeZones.localToUtc(testZoneName, local.millis) - utc.millis
      if (diff != 0)  fail("Calculated time differs "+ (diff / MILLIS_PER_MINUTE) +" minutes")
    }
  }

  for (((t, zoneName, withMills), s) <- Seq(
    (new LocalDateTime(2012, 10, 21, 11, 22), testZoneName, false) -> "2012-10-21 11:22:00+0300",
    (new LocalDateTime(2012, 10, 21, 11, 22, 33, 444), testZoneName, true) -> "2012-10-21 11:22:33.444+0300",
    (new LocalDateTime(2012, 12, 21, 11, 22, 33, 444), testZoneName, true) -> "2012-12-21 11:22:33.444+0200",
    (new LocalDateTime(2012, 10, 21, 11, 22, 33, 444), "UTC", true) -> "2012-10-21 11:22:33.444Z")) {
    test("toString "+t+" -> "+s) {
      TimeZones.toString(zoneName, withMills, t.toDateTime(DateTimeZone.forID(zoneName)).getMillis) should equal (s)
    }
  }

  private val springUtcToHelsinki =
      Utc(2012,  3, 25,  0, 30) -> Helsinki(2012,  3, 25, 2, 30) -> 2 ::    // Noch Winterzeit
      Utc(2012,  3, 25,  1,  0) -> Helsinki(2012,  3, 25, 4,  0) -> 3 ::    // Beginn der Sommerzeit, Sprung von 3 auf 4 Uhr
      Utc(2012,  3, 25,  1, 30) -> Helsinki(2012,  3, 25, 4, 30) -> 3 ::
      Nil
  private val fallUtcToHelsinki =
      Utc(2012, 10, 27, 23, 30) -> Helsinki(2012, 10, 28, 2, 30) -> 3 ::    // Noch Sommerzeit
      Utc(2012, 10, 28,  0,  0) -> Helsinki(2012, 10, 28, 3,  0) -> 3 ::
      Utc(2012, 10, 28,  0, 30) -> Helsinki(2012, 10, 28, 3, 30) -> 3 ::
      Utc(2012, 10, 28,  1,  0) -> Helsinki(2012, 10, 28, 3,  0) -> 2 ::    // Ende der Sommerzeit, Sprung von 4 auf 3 Uhr
      Utc(2012, 10, 28,  1, 30) -> Helsinki(2012, 10, 28, 3, 30) -> 2 ::
      Nil

  for (((utc, local), offsetHours) <- springUtcToHelsinki ++ fallUtcToHelsinki) {
    test(utc +" should yield "+ local) {
      local.millis - utc.millis should equal (offsetHours*MILLIS_PER_HOUR)
      val diff = TimeZones.utcToLocal(testZoneName, utc.millis) - local.millis
      if (diff != 0)  fail("Calculated time differs "+ (diff / MILLIS_PER_MINUTE) +" minutes")
    }
  }
}

object TimeZonesTest {
  private val testZoneName = "Europe/Helsinki"

  private trait Time {
    val year, month, day, hour, minute: Int
    def timeZoneName: String
    def millis: Long
    override def toString = new LocalDateTime(year, month, day, hour, minute).toString("yyyy-MM-dd HH:mm") +
        " "+ timeZoneName + " ("+(millis/MILLIS_PER_MINUTE)+")"
  }

  private case class Utc(year: Int, month: Int, day: Int, hour: Int, minute: Int) extends Time {
    def timeZoneName = "UTC"
    val millis = new DateTime(year, month, day, hour, minute, UTC).getMillis
  }

  private case class Helsinki(year: Int, month: Int, day: Int, hour: Int, minute: Int) extends Time {
    val timeZoneName = testZoneName
    val millis = new DateTime(year, month, day, 0, 0, UTC).getMillis + hour*MILLIS_PER_HOUR + minute*MILLIS_PER_MINUTE
  }
}
