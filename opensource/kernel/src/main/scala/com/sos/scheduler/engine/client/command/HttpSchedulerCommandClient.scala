package com.sos.scheduler.engine.client.command

import akka.actor.ActorSystem
import com.sos.scheduler.engine.client.command.HttpSchedulerCommandClient._
import javax.inject.{Inject, Singleton}
import scala.concurrent.Future
import scala.concurrent.duration._
import spray.client.pipelining._
import spray.http.MediaTypes.`application/xml`
import spray.http.Uri.Path
import spray.http.{HttpEntity, HttpRequest, Uri}
import spray.httpx.encoding.{Deflate, Gzip}
import spray.httpx.marshalling.Marshaller

/**
 * HTTP client for JobScheduler command webservice.
 * @author Joacim Zschimmer
 */
final class HttpSchedulerCommandClient private[command](baseUri: Uri)(implicit actorSystem: ActorSystem) {

  import actorSystem.dispatcher
  private implicit val timeout = 60.seconds
  private val commandUri = baseUri.path match {
    case Path.Empty | Path.SingleSlash ⇒ baseUri.copy(path = Path("/jobscheduler/engine/command"))
    case _ ⇒ throw new IllegalArgumentException(s"Invalid JobScheduler URL: $baseUri")
  }
  private val pipeline: HttpRequest ⇒ Future[String] =
    // JettyPlugin dekomprimiert Request nicht: encode(Gzip) ~>(  // Klammerung überlistet IntelliJ Scala 0.38.441
    sendReceive ~>
    decode(Deflate) ~>
    decode(Gzip) ~>
    unmarshal[String]

  /**
   * @return XML response
   */
  def executeXml(xmlBytes: Array[Byte]): Future[String] =
    pipeline(Post(commandUri, XmlBytes(xmlBytes))(XmlBytesMarshaller))

  /**
   * @return XML response
   */
  def execute(elem: xml.Elem): Future[String] =
    pipeline(Post(commandUri, elem))
}

object HttpSchedulerCommandClient {

  @Singleton
  final class Factory @Inject private(actorSystem: ActorSystem) {
    def apply(uri: java.net.URI) = new HttpSchedulerCommandClient(Uri(uri.toString))(actorSystem)

    def forUri(uri: String) = new HttpSchedulerCommandClient(Uri(uri))(actorSystem)
  }

  final case class XmlBytes(bytes: Array[Byte])

  private implicit val XmlBytesMarshaller = Marshaller.of[XmlBytes](`application/xml`) { (value, contentType, ctx) ⇒
    val XmlBytes(b) = value
    ctx.marshalTo(HttpEntity(contentType, b))
  }
}
