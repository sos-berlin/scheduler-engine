package com.sos.scheduler.engine.common.time

import ScalaJoda._
import org.joda.time.{LocalTime, DateTime, Instant, Duration}
import org.scalatest.FunSpec
import org.scalatest.matchers.ShouldMatchers._

class ScalaJodaTest extends FunSpec {
  describe("Duration") {
    it("Int.ms") {
      (7.ms: Duration).getMillis should equal (7)
    }
    it("Long.ms") {
      (7L.ms: Duration).getMillis should equal (7)
    }
    it("Int.s") {
      (7.s: Duration).getStandardSeconds should equal (7)
      (7.s: Duration).getMillis should equal (7*1000)
    }
    it("Long.s") {
      (7L.s: Duration).getStandardSeconds should equal (7)
      (7L.s: Duration).getMillis should equal (7*1000)
    }
    it("Int.hours") {
      (7.hours: Duration).getStandardHours should equal (7)
      (7.hours: Duration).getMillis should equal (7*3600*1000)
    }
    it("Long.hours") {
      (7L.hours: Duration).getStandardHours should equal (7)
      (7L.hours: Duration).getMillis should equal (7*3600*1000)
    }
    it("Int.days") {
      (7.days: Duration).getStandardDays should equal (7)
      (7.days: Duration).getMillis should equal (7*24*3600*1000)
    }
    it("Long.days") {
      (7L.days: Duration).getStandardDays should equal (7)
      (7L.days: Duration).getMillis should equal (7*24*3600*1000)
    }
    it("Duration + Duration") {
      ((7.s + 2.ms): Duration).getMillis should equal (7*1000 + 2)
    }
    it("Duration - Duration") {
      ((7.s - 2.ms): Duration).getMillis should equal (7*1000 - 2)
    }
    it("Int * Duration") {
      ((3 * 7.s): Duration).getMillis should equal (3 * 7*1000)
    }
    it("Long * Duration") {
      ((3L * 7.s): Duration).getMillis should equal (3 * 7*1000)
    }
//    it("Duration * Int") {
//      ((7.s * 3): Duration).getMillis should equal (7*1000 * 3)
//    }
//    it ("Duration < Duration") {
//      (new Duration(7) < new Duration(2)) should equal (false)
//      (new Duration(7) <= new Duration(2)) should equal (false)
//      (new Duration(7) > new Duration(2)) should equal (true)
//      (new Duration(7) >= new Duration(2)) should equal (true)
//      (new Duration(2) < new Duration(7)) should equal (true)
//      (new Duration(2) <= new Duration(7)) should equal (true)
//      (new Duration(2) > new Duration(2)) should equal (false)
//      (new Duration(2) >= new Duration(2)) should equal (false)
//      (new Duration(7) < new Duration(7)) should equal (false)
//      (new Duration(7) <= new Duration(7)) should equal (true)
//      (new Duration(7) > new Duration(2)) should equal (false)
//      (new Duration(7) >= new Duration(2)) should equal (true)
//    }
  }
  describe("Instant") {
    it ("Instant + Duration") {
      ((new Instant(7) + 2.ms): Instant) should equal (new Instant(7 + 2))
    }
    it ("Instant - Duration") {
      ((new Instant(7) - 2.ms): Instant) should equal (new Instant(7 - 2))
    }
    it ("Instant - Instant") {
      ((new Instant(7) - new Instant(2)): Duration) should equal (new Duration(7 - 2))
    }
  }
  describe("DateTime") {
    it ("DateTime + Duration") {
      ((new DateTime(7) + 2.ms): DateTime) should equal (new DateTime(7 + 2))
    }
    it ("DateTime - Duration") {
      ((new DateTime(7) - 2.ms): DateTime) should equal (new DateTime(7 - 2))
    }
    it ("DateTime - DateTime") {
      ((new DateTime(7) - new DateTime(2)): Duration) should equal (new Duration(7 - 2))
    }
  }
  describe("LocalTime") {
    it ("LocalTime < LocalTime") {
      (new LocalTime(7) < new LocalTime(2)) should equal (false)
      (new LocalTime(7) <= new LocalTime(2)) should equal (false)
      (new LocalTime(7) > new LocalTime(2)) should equal (true)
      (new LocalTime(7) >= new LocalTime(2)) should equal (true)

      (new LocalTime(2) < new LocalTime(7)) should equal (true)
      (new LocalTime(2) <= new LocalTime(7)) should equal (true)
      (new LocalTime(2) > new LocalTime(7)) should equal (false)
      (new LocalTime(2) >= new LocalTime(7)) should equal (false)

      (new LocalTime(7) < new LocalTime(7)) should equal (false)
      (new LocalTime(7) <= new LocalTime(7)) should equal (true)
      (new LocalTime(7) > new LocalTime(7)) should equal (false)
      (new LocalTime(7) >= new LocalTime(7)) should equal (true)
    }
  }
}
