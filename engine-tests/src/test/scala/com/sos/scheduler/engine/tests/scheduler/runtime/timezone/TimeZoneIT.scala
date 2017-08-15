package com.sos.scheduler.engine.tests.scheduler.runtime.timezone

import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.scheduler.runtime.timezone.TimeZoneIT._
import org.joda.time.DateTimeZone.UTC
import org.joda.time.format.DateTimeFormat
import org.joda.time.{DateTime, DateTimeZone, LocalTime}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class TimeZoneIT extends FreeSpec with ScalaSchedulerTest {

  private lazy val now = new DateTime
  private lazy val calendarEntryMap: Map[SchedulerObjectId, CalendarEntry] = (fetchCalendarEntries() map { e ⇒ e.obj → e }).toMap

  // Tests may fail during sommer time switch
  for (e ← expectedTimes) {
    val expected = e.nextDateTime(now)
    s"${e.obj} should have the start time $expected" in {
      val t = calendarEntryMap(e.obj)
      if (t.at.getMillis != expected.getMillis)  fail(s"<show_calendar> returns $t")
    }
  }

  private def fetchCalendarEntries() = {
    val tomorrow = now plusDays 1
    val calendar = scheduler executeXml <show_calendar what="jobs orders" from={dateTimeToXml(now)} before={dateTimeToXml(tomorrow)}/>
    calendar.elem \ "answer" \ "calendar" \ "period" map { node ⇒ CalendarEntry(node.asInstanceOf[xml.Elem]) }
  }
}

private object TimeZoneIT {
  private type SchedulerObjectId = Any

  private val expectedTimes = List(
      ExpectedObjectTime(JobPath("/a")      , new LocalTime(12, 12), DateTimeZone.forID("Pacific/Honolulu")),
      ExpectedObjectTime(OrderKey("/A", "1"), new LocalTime(12, 12), DateTimeZone.forID("Asia/Shanghai")))

  private def dateTimeToXml(o: DateTime) = o.toDateTime(UTC).toString(DateTimeFormat.forPattern("yyyy-MM-dd'T'HH:mm:ss'Z'"))

  private case class ExpectedObjectTime(obj: SchedulerObjectId, time: LocalTime, zone: DateTimeZone) {
    def nextDateTime(from: DateTime) = {
      val z = from.toDateTime(zone)
      val r = new DateTime(z.getYear, z.getMonthOfYear, z.getDayOfMonth, 0, 0, zone) plusMillis time.getMillisOfDay
      if (r isBefore from) r plusDays 1 else r
    }
  }

  private case class CalendarEntry(obj: SchedulerObjectId, at: DateTime, elem: xml.Elem)

  private object CalendarEntry {
    def apply(atElem: xml.Elem) = {
      def atDateTime = DateTime.parse(atElem.attribute("single_start").get.text)
      atElem.attribute("job") match {
        case Some(a) ⇒ new CalendarEntry(JobPath(a.text), atDateTime, atElem)
        case None ⇒ new CalendarEntry(OrderKey(atElem.attribute("job_chain").get.text, atElem.attribute("order").get.text), atDateTime, atElem)
      }
    }
  }
}
