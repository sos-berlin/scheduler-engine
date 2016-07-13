package com.sos.scheduler.engine.client.web

import akka.actor.ActorRefFactory
import akka.util.ByteString
import com.sos.scheduler.engine.client.api.CommandClient
import com.sos.scheduler.engine.client.common.RemoteSchedulers.checkResponseForError
import com.sos.scheduler.engine.client.web.WebCommandClient._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.xmls.{SafeXML, StringSource}
import com.sos.scheduler.engine.common.sprayutils.{XmlBytes, XmlString}
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
    // JettyPlugin dekomprimiert Request nicht: encode(Gzip) ~>
    addHeader(Accept(`application/xml`, `text/xml`)) ~>
      sendReceive ~>
      decode(Deflate) ~>
      decode(Gzip) ~>
      unmarshal[XmlString]

  final def executeXml(string: String): Future[String] =
    uncheckedExecuteXml(string) map { response ⇒
      checkResponseForError(StringSource(response))
      response
    }

  final def executeXml(byteString: ByteString): Future[String] =
    uncheckedExecuteXml(byteString) map { response ⇒
      checkResponseForError(StringSource(response))
      response
    }

  final def execute(elem: xml.Elem): Future[String] =
    uncheckedExecute(elem) map { response ⇒
      checkResponseForError(StringSource(response))
      response
    }

  final def uncheckedExecuteXml(string: String): Future[String] = {
    logger.debug(s"POST $commandUri $string}")
    pipeline(Post(commandUri, XmlString(string))) map { _.string }
  }

  final def uncheckedExecuteXml(byteString: ByteString): Future[String] = {
    logger.debug(s"POST $commandUri ${try SafeXML.load(new ByteArrayInputStream(byteString.toArray)) catch { case NonFatal(t) ⇒ t }}")
    pipeline(Post(commandUri, XmlBytes(byteString))) map { _.string }
  }

  final def uncheckedExecute(elem: xml.Elem): Future[String] = {
    logger.debug(s"POST $commandUri $elem")
    pipeline(Post(commandUri, elem)) map { _.string }
  }
}

object WebCommandClient {
  private val logger = Logger(getClass)
}
