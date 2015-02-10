package com.sos.scheduler.engine.client.command

import com.sos.scheduler.engine.common.scalautil.Collections.implicits._
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.scheduler.engine.data.message.MessageCode
import javax.xml.transform.Source
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
object RemoteSchedulers {

  def checkResponseForError(source: Source): Unit = readSchedulerResponses(source)(readAnythingAndIgnore)

  def readSchedulerResponse[A](source: Source)(read: ScalaXMLEventReader ⇒ A): A = {
    val answers = readSchedulerResponses(source)(read)
    if (answers.isEmpty) sys.error("Incomplete XML response")
    if (answers.size > 1) sys.error(s"More than one (${answers.size}) XML response")
    answers.head
  }
  
  def readSchedulerResponses[A](source: Source)(read: ScalaXMLEventReader ⇒ A): immutable.Seq[A] = {
    try {
      val result = ScalaXMLEventReader.parse(source) { eventReader ⇒
        import eventReader._
        parseDocument {
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
      }
      result.values.toImmutableSeq
    } catch {
      case ScalaXMLEventReader.XmlException(x: XmlResponseException) ⇒ throw x
    }
  }

  def errorElementToException(eventReader: ScalaXMLEventReader): XmlResponseException = {
    import eventReader._
    val (code, text) = parseElement("ERROR") {
      attributeMap.ignoreUnread()
      (attributeMap.getAsConverted("code")(MessageCode.apply), attributeMap.get("text"))
    }
    throw new XmlResponseException(
      code = code getOrElse MessageCode("UNKNOWN"),
      getMessage = text getOrElse "UNKNOWN ERROR")
  }

  final case class XmlResponseException(code: MessageCode, override val getMessage: String) extends RuntimeException

  private def readAnythingAndIgnore(eventReader: ScalaXMLEventReader): Unit = {
    import eventReader._
    forEachStartElement {
      case _ ⇒ parseElement() {
        attributeMap.ignoreUnread()
        readAnythingAndIgnore(eventReader)
      }
    }
  }
}
