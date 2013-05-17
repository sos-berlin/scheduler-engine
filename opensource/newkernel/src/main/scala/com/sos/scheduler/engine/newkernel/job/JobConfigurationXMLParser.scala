package com.sos.scheduler.engine.newkernel.job

import com.sos.scheduler.engine.common.scalautil.xml.ScalaXMLEventReader
import com.sos.scheduler.engine.newkernel.schedule.oldruntime.OldScheduleXMLParser
import java.io.StringReader
import javax.xml.stream.events.{EndDocument, StartDocument}
import javax.xml.stream.{XMLInputFactory, XMLEventReader}
import javax.xml.transform.Source
import javax.xml.transform.stream.StreamSource
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
    var language = ""
    parseElement("script") {
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
  def parse(xml: String, inputFactory: XMLInputFactory, timeZone: DateTimeZone): JobConfiguration =
    parseDocument(new StreamSource(new StringReader(xml)), inputFactory, timeZone)

  def parseDocument(source: Source, inputFactory: XMLInputFactory, timeZone: DateTimeZone): JobConfiguration = {
    val reader = inputFactory.createXMLEventReader(source)
    reader.nextEvent().asInstanceOf[StartDocument]
    val parser = new JobConfigurationXMLParser(timeZone, reader)
    val result = parser.parse()
    reader.nextEvent().asInstanceOf[EndDocument]
    result
  }
}