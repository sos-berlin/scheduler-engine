package com.sos.scheduler.engine.kernel.time

import org.joda.time.{LocalDateTime, DateTime}
import org.joda.time.DateTimeConstants._
import org.joda.time.DateTimeZone.UTC
import org.scalatest.FunSuite
import org.scalatest.matchers.ShouldMatchers._

final class TimeZonesTest extends FunSuite {
  import TimeZonesTest._

  private val springHelsinkiToUtc =
      Helsinki(2012,  3, 25, 2, 30) -> Utc(2012,  3, 25,  0, 30) -> Some(2) ::    // Noch Winterzeit
      Helsinki(2012,  3, 25, 3,  0) -> Utc(2012,  3, 25,  1,  0) -> Some(2) ::    // Beginn der Sommerzeit, Sprung von 3 auf 4 Uhr
      Helsinki(2012,  3, 25, 3, 30) -> Utc(2012,  3, 25,  1,  0) -> None ::       // Diese lokale Zeit gibt es nicht, die Studen fehlt
      Helsinki(2012,  3, 25, 4,  0) -> Utc(2012,  3, 25,  1,  0) -> Some(3) ::
      Helsinki(2012,  3, 25, 4, 30) -> Utc(2012,  3, 25,  1, 30) -> Some(3) ::
      Nil
  private val fallHelsinkiToUtc =
      Helsinki(2012, 10, 28, 2, 30) -> Utc(2012, 10, 27, 23, 30) -> Some(3) ::    // +3h Noch Sommerzeit
      Helsinki(2012, 10, 28, 3,  0) -> Utc(2012, 10, 28,  0,  0) -> Some(3) ::    // +3h 2 bis 3 Uhr lokaler Zeit ist doppelt
      Helsinki(2012, 10, 28, 3, 30) -> Utc(2012, 10, 28,  0, 30) -> Some(3) ::    // +3h
      Helsinki(2012, 10, 28, 4,  0) -> Utc(2012, 10, 28,  2,  0) -> Some(2) ::    // +2h Ende der Sommerzeit, Sprung von 4 auf 3 Uhr
      Helsinki(2012, 10, 28, 4, 30) -> Utc(2012, 10, 28,  2, 30) -> Some(2) ::    // +2h Winterzeit
      Nil

  springHelsinkiToUtc ++ fallHelsinkiToUtc foreach { case ((local, utc), offsetHoursOption) =>
    test(local +" should yield "+ utc) {
      offsetHoursOption foreach { h => local.millis - utc.millis should equal (h*MILLIS_PER_HOUR) }
      val diff = TimeZones.localToUtc(testZoneName, local.millis) - utc.millis
      if (diff != 0)  fail("Calculated time differs "+ (diff / MILLIS_PER_MINUTE) +" minutes")
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

  springUtcToHelsinki ++ fallUtcToHelsinki foreach { case ((utc, local), offsetHours) =>
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
