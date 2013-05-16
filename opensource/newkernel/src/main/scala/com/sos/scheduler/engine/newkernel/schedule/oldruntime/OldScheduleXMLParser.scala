package com.sos.scheduler.engine.newkernel.schedule.oldruntime

import com.sos.scheduler.engine.newkernel.schedule.{Weekday, Schedule}
import com.sos.scheduler.engine.newkernel.utils.ScalaXMLEventReader
import javax.xml.stream.XMLEventReader
import org.joda.time.DateTimeZone

final class OldScheduleXMLParser(defaultTimeZone: DateTimeZone, eventReader: XMLEventReader) {
  private val eventIterator = new ScalaXMLEventReader(eventReader)

  import eventIterator._

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
        case "weekdays" => parseAttributelessElement {
          forEachStartElement {
            case "day" => parseElement {
              forEachAttribute {
                case ("day", s) =>
                  builder.weekdaysPeriods(Weekday(s)) = SchedulePeriodXMLParser.parseSchedulePeriodSeq(eventReader)
              }
            }
          }
        }
      }
      builder.toRuntimeSchedule
    }
}
