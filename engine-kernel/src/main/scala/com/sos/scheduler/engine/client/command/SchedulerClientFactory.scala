package com.sos.scheduler.engine.client.command

import akka.actor.ActorSystem
import com.sos.scheduler.engine.client.command.RemoteSchedulers.checkResponseForError
import com.sos.scheduler.engine.client.command.SchedulerClientFactory._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.xmls.{SafeXML, StringSource}
import com.sos.scheduler.engine.common.sprayutils.XmlString
import java.io.ByteArrayInputStream
import javax.inject.{Inject, Singleton}
import scala.concurrent.Future
import scala.concurrent.duration._
import scala.util.control.NonFatal
import spray.client.pipelining._
import spray.http.HttpHeaders.Accept
import spray.http.MediaTypes._
import spray.http.Uri.Path
import spray.http.{HttpEntity, HttpRequest, Uri}
import spray.httpx.encoding.{Deflate, Gzip}
import spray.httpx.marshalling.Marshaller

/**
 * HTTP client for JobScheduler command webservice.
 *
 * @author Joacim Zschimmer
 */
@Singleton
final class SchedulerClientFactory @Inject private(implicit actorSystem: ActorSystem) {

  import actorSystem.dispatcher

  private implicit val timeout = 60.seconds

  private def commandUri(baseUri: Uri) = baseUri.path match {
    case Path.Empty | Path.SingleSlash ⇒ baseUri.copy(path = Path("/jobscheduler/engine/command"))
    case _ ⇒ throw new IllegalArgumentException(s"Invalid JobScheduler URL: $baseUri")
  }

  private val pipeline: HttpRequest ⇒ Future[XmlString] =
    // JettyPlugin dekomprimiert Request nicht: encode(Gzip) ~>
    addHeader(Accept(`application/xml`, `text/xml`)) ~>
      sendReceive ~>
      decode(Deflate) ~>
      decode(Gzip) ~>
      unmarshal[XmlString]

  def apply(baseUri: Uri): SchedulerCommandClient = new SchedulerCommandClient {
    def executeXml(xmlBytes: Array[Byte]): Future[String] =
      uncheckedExecuteXml(xmlBytes) map { response ⇒
        checkResponseForError(StringSource(response))
        response
      }

    def execute(elem: xml.Elem): Future[String] =
      uncheckedExecute(elem) map { response ⇒
        checkResponseForError(StringSource(response))
        response
      }

    def uncheckedExecuteXml(xmlBytes: Array[Byte]): Future[String] = {
      logger.debug(s"POST ${commandUri(baseUri)} ${try SafeXML.load(new ByteArrayInputStream(xmlBytes)) catch { case NonFatal(t) ⇒ t }})}")
      pipeline(Post(commandUri(baseUri), XmlBytes(xmlBytes))(XmlBytesMarshaller)) map { _.string }
    }

    def uncheckedExecute(elem: xml.Elem): Future[String] = {
      logger.debug(s"POST ${commandUri(baseUri)} $elem")
      pipeline(Post(commandUri(baseUri), elem)) map { _.string }
    }
  }
}

object SchedulerClientFactory {
  private val logger = Logger(getClass)

  final case class XmlBytes(bytes: Array[Byte])

  private val XmlBytesMarshaller = Marshaller.of[XmlBytes](`application/xml`) { (value, contentType, ctx) ⇒
    val XmlBytes(b) = value
    ctx.marshalTo(HttpEntity(contentType, b))
  }
}
