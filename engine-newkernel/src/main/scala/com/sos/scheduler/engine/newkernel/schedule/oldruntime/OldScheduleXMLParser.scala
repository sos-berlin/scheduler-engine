package com.sos.scheduler.engine.newkernel.schedule.oldruntime

import com.sos.jobscheduler.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.scheduler.engine.newkernel.schedule.{Schedule, Weekday}
import org.joda.time.DateTimeZone

final class OldScheduleXMLParser(defaultTimeZone: DateTimeZone, eventReader: ScalaXMLEventReader) {

  import eventReader._

  def parse(): Schedule =
    parseElement("run_time") {
      val builder = new OldSchedule.Builder
      builder.timeZone = defaultTimeZone
      val defaultPeriodBuilder = new OldPeriod.Builder()
      forEachAttribute {
        SchedulePeriodXMLParser.parsePeriodAttributeInto(defaultPeriodBuilder) orElse {
          case ("time_zone", s) => builder.timeZone = DateTimeZone forID s
        }
      }
      builder.startOnce = defaultPeriodBuilder.once  // Attribut "once" gehört nicht zu <period>, sondern zu unserem <run_time>
      defaultPeriodBuilder.once = false

      forEachStartElement {
        case "period" => builder.periods += new SchedulePeriodXMLParser(eventReader).parse()
        case "weekdays" => parseElement() {
          forEachStartElement {
            case "day" => parseElement() {
              builder.weekdaysPeriods(Weekday(attributeMap("day"))) = SchedulePeriodXMLParser.parseSchedulePeriodSeq(eventReader)
            }
          }
        }
      }
      builder.toRuntimeSchedule
    }
}
