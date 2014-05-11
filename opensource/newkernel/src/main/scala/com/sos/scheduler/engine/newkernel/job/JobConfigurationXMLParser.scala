package com.sos.scheduler.engine.newkernel.job

import com.sos.scheduler.engine.common.scalautil.xml.ScalaXMLEventReader
import com.sos.scheduler.engine.newkernel.schedule.oldruntime.OldScheduleXMLParser
import javax.xml.stream.{XMLInputFactory, XMLEventReader}
import javax.xml.transform.Source
import org.joda.time.DateTimeZone
import scala.sys.error

class JobConfigurationXMLParser(timeZone: DateTimeZone, eventReader: XMLEventReader) {
  private val eventIterator = new ScalaXMLEventReader(eventReader)

  import eventIterator._

  def parse(): JobConfiguration =
    parseElement("new_job") {
      val builder = new JobConfiguration.Builder
      forEachAttribute {
        case ("title", v) => builder.title = v
      }
      forEachStartElement {
        case "description" => parseAttributelessElement { builder.description = eventIterator.eatText() }
        case "run_time" => builder.schedule = Some(new OldScheduleXMLParser(timeZone, eventReader).parse())
        case "script" => builder.script = Some(parseScript())
      }
      builder.build()
    }

  private def parseScript(): JobScript = {
    parseElement("script") {
      var language = ""
      forEachAttribute {
        case ("language", v) => language = v
      }
      val text = eatText().trim
      language match {
        case "shell" => ShellScript(text)
        case _ => error(s"Unknown script language '$language'")
      }
    }
  }
}

object JobConfigurationXMLParser {
  def parseString(xml: String, inputFactory: XMLInputFactory, timeZone: DateTimeZone): JobConfiguration =
    ScalaXMLEventReader.parseString(xml, inputFactory)(parseWithReader(timeZone))

  def parseDocument(source: Source, inputFactory: XMLInputFactory, timeZone: DateTimeZone): JobConfiguration =
    ScalaXMLEventReader.parseDocument(source, inputFactory)(parseWithReader(timeZone))

  def parseWithReader(timeZone: DateTimeZone)(reader: XMLEventReader) =
    new JobConfigurationXMLParser(timeZone, reader).parse()
}
