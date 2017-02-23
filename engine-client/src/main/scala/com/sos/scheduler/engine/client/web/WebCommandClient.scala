package com.sos.scheduler.engine.client.web

import akka.actor.ActorRefFactory
import akka.util.ByteString
import com.sos.jobscheduler.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.common.scalautil.xmls.SafeXML
import com.sos.jobscheduler.common.sprayutils.{XmlBytes, XmlString}
import com.sos.scheduler.engine.client.api.CommandClient
import com.sos.scheduler.engine.client.web.WebCommandClient._
import java.io.ByteArrayInputStream
import scala.concurrent.Future
import scala.concurrent.duration._
import scala.util.control.NonFatal
import spray.client.pipelining._
import spray.http.HttpHeaders.Accept
import spray.http.MediaTypes._
import spray.http.{HttpRequest, Uri}
import spray.httpx.encoding.{Deflate, Gzip}

/**
  * @author Joacim Zschimmer
  */
trait WebCommandClient extends CommandClient {

  protected def commandUri: Uri
  protected implicit val actorRefFactory: ActorRefFactory

  import actorRefFactory.dispatcher

  private implicit val timeout = 60.seconds

  private lazy val pipeline: HttpRequest ⇒ Future[XmlString] =
    // JettyPlugin (bis v1.10) dekomprimiert Request nicht: encode(Gzip) ~>
    addHeader(Accept(`application/xml`, `text/xml`)) ~>
      sendReceive ~>
      decode(Deflate) ~>
      decode(Gzip) ~>
      unmarshal[XmlString]

  final def execute(elem: xml.Elem): Future[String] =
    uncheckedExecute(elem) map { response ⇒
      checkResponseForError(response)
      response
    }

  final def executeXml(byteString: ByteString): Future[String] =
    uncheckedExecuteXml(byteString) map { response ⇒
      checkResponseForError(response)
      response
    }

  final def executeXml(string: String): Future[String] =
    uncheckedExecuteXml(string) map { response ⇒
      checkResponseForError(response)
      response
    }

  final def uncheckedExecute(elem: xml.Elem): Future[String] = {
    logger.debug(s"POST $commandUri $elem")
    pipeline(Post(commandUri, elem)) map { _.string }
  }

  final def uncheckedExecuteXml(byteString: ByteString): Future[String] = {
    logger.debug(s"POST $commandUri ${try SafeXML.load(new ByteArrayInputStream(byteString.toArray)) catch { case NonFatal(t) ⇒ t }}")
    pipeline(Post(commandUri, XmlBytes(byteString))) map { _.string }
  }

  final def uncheckedExecuteXml(string: String): Future[String] = {
    logger.debug(s"POST $commandUri $string}")
    pipeline(Post(commandUri, XmlString(string))) map { _.string }
  }
}

object WebCommandClient {
  private val logger = Logger(getClass)

  // Similar to RemoteSchedulers.checkResponseForError, but accepts text in XML and is faster
  def checkResponseForError(xmlString: String): Unit = {
    xmlString.contains("<ERROR") option  {
      val error = SafeXML.loadString(xmlString) \ "answer" \ "ERROR"
      //val code = (error \ "@code").toString
      val message = (error \ "@text").toString
      throw new XmlException(message)
    }
  }

  final class XmlException(message: String) extends RuntimeException(message)
}
