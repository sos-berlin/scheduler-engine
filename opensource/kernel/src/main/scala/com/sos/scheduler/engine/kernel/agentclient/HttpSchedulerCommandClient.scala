package com.sos.scheduler.engine.kernel.agentclient

import akka.actor.ActorSystem
import javax.inject.{Inject, Singleton}
import scala.concurrent.Future
import scala.concurrent.duration._
import spray.client.pipelining._
import spray.http.Uri.Path
import spray.http.{HttpRequest, Uri}
import spray.httpx.encoding.{Deflate, Gzip}

/**
 * HTTP client for JobScheduler command webservice.
 * @author Joacim Zschimmer
 */
final class HttpSchedulerCommandClient(baseUri: Uri)(implicit actorSystem: ActorSystem) {

  import actorSystem.dispatcher
  private implicit val timeout = 60.seconds
  private val commandUri = baseUri.path match {
    case Path.Empty | Path.SingleSlash ⇒ baseUri.copy(path = Path("/jobscheduler/engine/command"))
    case _ ⇒ sys.error(s"Invalid JobScheduler URI: $baseUri")
  }
  private val pipeline: HttpRequest ⇒ Future[String] =
    sendReceive ~>
    decode(Deflate) ~>
    decode(Gzip) ~>
    unmarshal[String]

  /**
   * @return XML response
   */
  def execute(elem: xml.Elem): Future[String] =
    pipeline(Post(commandUri, elem))
}

private object HttpSchedulerCommandClient {

  @Singleton
  final class Factory @Inject private(actorSystem: ActorSystem) {
    def apply(uri: java.net.URI) = new HttpSchedulerCommandClient(Uri(uri.toString))(actorSystem)
  }
}
