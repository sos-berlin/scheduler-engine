package com.sos.scheduler.engine.client.common

import com.sos.jobscheduler.base.convert.As
import com.sos.jobscheduler.common.scalautil.Collections.implicits._
import com.sos.jobscheduler.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.jobscheduler.data.message.MessageCode
import javax.xml.transform.Source
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
object RemoteSchedulers {

  def readSchedulerResponse[A](source: Source)(read: ScalaXMLEventReader ⇒ A): A = {
    val answers = readSchedulerResponses(source)(read)
    if (answers.isEmpty) sys.error("Incomplete XML response")
    if (answers.size > 1) sys.error(s"More than one (${answers.size}) XML response")
    answers.head
  }

  def readSchedulerResponses[A](source: Source)(read: ScalaXMLEventReader ⇒ A): immutable.Seq[A] = {
    try {
      val result = ScalaXMLEventReader.parseDocument(source) { eventReader ⇒
        import eventReader._
        parseElement("spooler") {
          parseElement("answer") {
            attributeMap.ignoreUnread()  // Attribut "time"
            forEachStartElement[A] {
              case "ERROR" ⇒ throw errorElementToException(eventReader)
              case _ ⇒ read(eventReader)
            }
          }
        }
      }
      result.values
    } catch {
      case ScalaXMLEventReader.XmlException(x: XmlResponseException) ⇒ throw x
    }
  }

  def errorElementToException(eventReader: ScalaXMLEventReader): XmlResponseException = {
    import eventReader._
    val (code, text) = parseElement("ERROR") {
      attributeMap.ignoreUnread()
      (attributeMap.optionAs("code")(As(MessageCode)), attributeMap.get("text"))
    }
    throw new XmlResponseException(
      code = code getOrElse MessageCode("UNKNOWN"),
      getMessage = text getOrElse "UNKNOWN ERROR")
  }

  final class XmlResponseException(code: MessageCode, override val getMessage: String) extends RuntimeException
}
