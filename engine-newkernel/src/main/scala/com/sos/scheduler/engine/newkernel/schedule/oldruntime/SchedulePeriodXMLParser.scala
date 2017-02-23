package com.sos.scheduler.engine.newkernel.schedule.oldruntime

import SchedulePeriodXMLParser._
import com.sos.jobscheduler.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.scheduler.engine.newkernel.schedule.TimeOfDay
import com.sos.scheduler.engine.newkernel.utils.StringConverters._
import org.joda.time.Duration.standardSeconds
import scala.collection.mutable

class SchedulePeriodXMLParser(eventReader: ScalaXMLEventReader) {

  import eventReader._

  def parse(): OldPeriod =
    parseElement("period") {
      val builder = new OldPeriod.Builder()
      forEachAttribute { parsePeriodAttributeInto(builder) }
      builder.toSchedulePeriod
    }
}

object SchedulePeriodXMLParser {
  def parsePeriodAttributeInto(builder: OldPeriod.Builder): PartialFunction[(String, String), Unit] = {
    case ("begin", s) => builder.begin = TimeOfDay(s)
    case ("end", s) => builder.end = TimeOfDay(s)
    case ("repeat", s) => builder.repeat = Some(standardSeconds(s.toInt))
    case ("absolute_repeat", s) => builder.absoluteRepeat = Some(standardSeconds(s.toInt))
    case ("once", s) => builder.once = stringToBoolean(s)
  }

  def parseSchedulePeriodSeq(eventReader: ScalaXMLEventReader): PeriodSeq = {
    val buffer = mutable.Buffer[OldPeriod]()
    val periodParser = new SchedulePeriodXMLParser(eventReader)
    while (eventReader.peek.isStartElement) {
      buffer += periodParser.parse()
    }
    PeriodSeq(buffer)
  }
}
