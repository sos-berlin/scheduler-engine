package com.sos.scheduler.engine.tests.scheduler.runtime.timezone

import com.sos.scheduler.engine.data.folder.JobPath
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.joda.time.{DateTimeZone, LocalTime, DateTime}
import org.joda.time.DateTimeZone.UTC
import org.joda.time.format.DateTimeFormat
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import scala.xml.Elem

@RunWith(classOf[JUnitRunner])
final class TimeZoneTest extends ScalaSchedulerTest {
  import TimeZoneTest._

  private lazy val now = new DateTime
  private lazy val calendarEntryMap: Map[SchedulerObjectId, DateTime] = (fetchCalendarEntries() map { e => e.obj -> e.at }).toMap

  // Die Tests kann während einer Sommerzeitverschiebung fehlschlagen.
  expectedTimes foreach { e =>
    val expected = e.nextDateTime(now)
    test(e.obj +" should have the start time "+ expected) {
      val t = calendarEntryMap(e.obj)
      if (t.getMillis != expected.getMillis)  fail("<show_calendar> returns "+t)
    }
  }

  private def fetchCalendarEntries() = {
    val tomorrow = now plusDays 1
    val calendarElem = scheduler executeXml <show_calendar what="jobs orders" from={dateTimeToXml(now)} before={dateTimeToXml(tomorrow)}/>
    calendarElem \ "answer" \ "calendar" \ "at" map { node => CalendarEntry(node.asInstanceOf[Elem]) }
  }
}

object TimeZoneTest {
  type SchedulerObjectId = Any

  private val expectedTimes = List(
      ExpectedObjectTime(JobPath.of("/a")      , new LocalTime(12, 12), DateTimeZone.forID("Pacific/Honolulu")),
      ExpectedObjectTime(OrderKey.of("/A", "1"), new LocalTime(12, 12), DateTimeZone.forID("Asia/Shanghai")))

  private def dateTimeToXml(o: DateTime) = o.toDateTime(UTC).toString(DateTimeFormat.forPattern("yyyy-MM-dd'T'HH:mm:ss'Z'"))

  private case class ExpectedObjectTime(obj: SchedulerObjectId, time: LocalTime, zone: DateTimeZone) {
    def nextDateTime(from: DateTime) = {
      val z = from.toDateTime(zone)
      val r = new DateTime(z.getYear, z.getMonthOfYear, z.getDayOfMonth, 0, 0, zone) plusMillis time.getMillisOfDay
      if (r isBefore from) r plusDays 1 else r
    }
  }

  private case class CalendarEntry(obj: SchedulerObjectId, at: DateTime)

  private object CalendarEntry {
    def apply(atElem: Elem) = {
      def atDateTime = DateTime.parse(atElem.attribute("at").get.text)
      if (atElem.attribute("job").isDefined)
        new CalendarEntry(JobPath.of(atElem.attribute("job").get.text), atDateTime)
      else
        new CalendarEntry(OrderKey.of(atElem.attribute("job_chain").get.text, atElem.attribute("order").get.text), atDateTime)
    }
  }
}
