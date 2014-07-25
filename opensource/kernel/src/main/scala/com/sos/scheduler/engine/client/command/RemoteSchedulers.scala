package com.sos.scheduler.engine.client.command

import com.sos.scheduler.engine.common.scalautil.xml.ScalaXMLEventReader
import com.sos.scheduler.engine.data.message.MessageCode
import javax.xml.transform.Source
import scala.collection.{immutable, mutable}

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
      val result = mutable.Buffer[A]()
      ScalaXMLEventReader.parse(source) { eventReader ⇒
        import eventReader._
        parseDocument {
          parseElement("spooler") {
            ignoreAttributes()
            parseElement("answer") {
              ignoreAttributes()
              forEachStartElement {
                case "ERROR" ⇒ throw errorElementToException(eventReader)
                case _ ⇒ result += read(eventReader)
              }
            }
          }
        }
      }
      result.toVector
    } catch {
      case ScalaXMLEventReader.WrappedException(x: XmlResponseException) ⇒ throw x
    }
  }

  def errorElementToException(eventReader: ScalaXMLEventReader): XmlResponseException = {
    import eventReader._
    var code: Option[MessageCode] = None
    var text: Option[String] = None
    parseElement("ERROR") {
      forEachAttribute {
        case ("code", value) ⇒ code = Some(MessageCode(value))
        case ("text", value) ⇒ text = Some(value)
        case _ ⇒
      }
    }
    throw new XmlResponseException(code getOrElse MessageCode("UNKNOWN"), text getOrElse "UNKNOWN ERROR")
  }

  final case class XmlResponseException(code: MessageCode, override val getMessage: String) extends RuntimeException
}
