package com.sos.scheduler.engine.tests.scheduler.runtime.substitute

import com.sos.scheduler.engine.common.scalautil.xmls.RichScalaXML._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.scheduler.runtime.substitute.SubstituteScheduleIT._
import org.joda.time.DateTimeZone
import org.joda.time.format.ISODateTimeFormat
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class SubstituteScheduleIT extends FunSuite with ScalaSchedulerTest {

  test("Schedule with substitute") {
    val response = scheduler executeXml <show_calendar from="2030-01-01T00:00:00" before="2030-01-07T00:00:00"/>
    val dateTimes = response.elem.descendant collect {
      case e: xml.Elem if e.label == "period" && e.attributeText("job") == jobPath.string =>
        val t = e.attributeText("single_start")
        ISODateTimeFormat.dateOptionalTimeParser.parseDateTime(t).toDateTime(DateTimeZone.getDefault)
    }
    dateTimes should equal (List(
      dt("2030-01-01T00:11"),
      dt("2030-01-01T23:22"),
      dt("2030-01-02T00:11"),
      dt("2030-01-02T23:22"),
        dt("2030-01-03T00:44"),
        dt("2030-01-03T23:55"),
        dt("2030-01-04T00:44"),
        dt("2030-01-04T23:55"),
      dt("2030-01-05T00:11"),
      dt("2030-01-05T23:22"),
      dt("2030-01-06T00:11"),
      dt("2030-01-06T23:22")))
  }
}

private object SubstituteScheduleIT {
  private val jobPath = JobPath("/a")

  private def dt(o: String) =
    ISODateTimeFormat.dateHourMinute().parseDateTime(o)
}
