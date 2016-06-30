package com.sos.scheduler.engine.client.web

import com.sos.scheduler.engine.client.common.RemoteSchedulers.checkResponseForError
import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.client.api.CommandClient
import com.sos.scheduler.engine.client.web.WebCommandClient._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.xmls.{SafeXML, StringSource}
import com.sos.scheduler.engine.common.sprayutils.XmlString
import java.io.ByteArrayInputStream
import scala.concurrent.Future
import scala.concurrent.duration._
import scala.util.control.NonFatal
import spray.client.pipelining._
import spray.http.HttpHeaders.Accept
import spray.http.MediaTypes._
import spray.http.{HttpEntity, HttpRequest, Uri}
import spray.httpx.encoding.{Deflate, Gzip}
import spray.httpx.marshalling.Marshaller

/**
  * @author Joacim Zschimmer
  */
trait WebCommandClient extends CommandClient {

  protected def schedulerUri: Uri
  protected implicit val actorRefFactory: ActorRefFactory

  import actorRefFactory.dispatcher

  private implicit val timeout = 60.seconds

  private val pipeline: HttpRequest ⇒ Future[XmlString] =
    // JettyPlugin dekomprimiert Request nicht: encode(Gzip) ~>
    addHeader(Accept(`application/xml`, `text/xml`)) ~>
      sendReceive ~>
      decode(Deflate) ~>
      decode(Gzip) ~>
      unmarshal[XmlString]

  final def executeXml(xmlBytes: Array[Byte]): Future[String] =
    uncheckedExecuteXml(xmlBytes) map { response ⇒
      checkResponseForError(StringSource(response))
      response
    }

  final def execute(elem: xml.Elem): Future[String] =
    uncheckedExecute(elem) map { response ⇒
      checkResponseForError(StringSource(response))
      response
    }

  final def uncheckedExecuteXml(xmlBytes: Array[Byte]): Future[String] = {
    logger.debug(s"POST ${commandUri(schedulerUri)} ${try SafeXML.load(new ByteArrayInputStream(xmlBytes)) catch { case NonFatal(t) ⇒ t }})}")
    pipeline(Post(commandUri(schedulerUri), XmlBytes(xmlBytes))(XmlBytesMarshaller)) map { _.string }
  }

  final def uncheckedExecute(elem: xml.Elem): Future[String] = {
    logger.debug(s"POST ${commandUri(schedulerUri)} $elem")
    pipeline(Post(commandUri(schedulerUri), elem)) map { _.string }
  }

  private def commandUri(baseUri: Uri) = baseUri.path match {
    case Uri.Path.Empty | Uri.Path.SingleSlash ⇒ baseUri.copy(path = Uri.Path("/jobscheduler/engine/command"))
    case _ ⇒ throw new IllegalArgumentException(s"Invalid JobScheduler URL: $baseUri")
  }
}

object WebCommandClient {
  private val logger = Logger(getClass)

  final case class XmlBytes(bytes: Array[Byte])

  private val XmlBytesMarshaller = Marshaller.of[XmlBytes](`application/xml`) { (value, contentType, ctx) ⇒
    val XmlBytes(b) = value
    ctx.marshalTo(HttpEntity(contentType, b))
  }
}
