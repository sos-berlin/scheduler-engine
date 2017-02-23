package com.sos.scheduler.engine.newkernel.job

import com.sos.jobscheduler.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.scheduler.engine.newkernel.schedule.oldruntime.OldScheduleXMLParser
import javax.xml.transform.Source
import org.joda.time.DateTimeZone

class JobConfigurationXMLParser(timeZone: DateTimeZone, eventReader: ScalaXMLEventReader) {

  import eventReader._

  def parse(): JobConfiguration =
    parseElement("new_job") {
      val builder = new JobConfiguration.Builder
      builder.title = attributeMap.getOrElse("title", "")
      forEachStartElement {
        case "description" => parseElement() { builder.description = eatText() }
        case "run_time" => builder.schedule = Some(new OldScheduleXMLParser(timeZone, eventReader).parse())
        case "script" => builder.script = Some(parseScript())
      }
      builder.build()
    }

  private def parseScript(): JobScript = {
    parseElement("script") {
      val language = attributeMap("language")
      val text = eatText().trim
      language match {
        case "shell" => ShellScript(text)
        case _ => sys.error(s"Unknown script language '$language'")
      }
    }
  }
}

object JobConfigurationXMLParser {
  def parseString(xml: String, timeZone: DateTimeZone): JobConfiguration =
    ScalaXMLEventReader.parseString(xml)(parseWithReader(timeZone))

  def parseDocument(source: Source, timeZone: DateTimeZone): JobConfiguration =
    ScalaXMLEventReader.parseDocument(source)(parseWithReader(timeZone))

  def parseWithReader(timeZone: DateTimeZone)(reader: ScalaXMLEventReader) =
    new JobConfigurationXMLParser(timeZone, reader).parse()
}
