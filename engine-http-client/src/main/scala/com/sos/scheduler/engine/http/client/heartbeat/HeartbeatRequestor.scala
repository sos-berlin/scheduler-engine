package com.sos.scheduler.engine.http.client.heartbeat

import akka.actor.ActorRefFactory
import akka.pattern.AskTimeoutException
import com.google.inject.ImplementedBy
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.http.client.heartbeat.HeartbeatRequestHeaders._
import com.sos.scheduler.engine.http.client.heartbeat.HeartbeatRequestor._
import java.time.Duration
import javax.inject.{Inject, Singleton}
import org.jetbrains.annotations.TestOnly
import scala.concurrent.{Future, blocking}
import spray.client.pipelining._
import spray.http.StatusCodes.Accepted
import spray.http.{HttpEntity, HttpRequest, HttpResponse}

/**
  * @author Joacim Zschimmer
  */
final class HeartbeatRequestor @Inject private[http](timing: HttpHeartbeatTiming, testWithHeartbeatDelay: Duration = 0.s) {

  private var _heartbeatCount = 0

  def close() = {}

  def apply(sendReceive: SendReceive, httpRequest: HttpRequest)(implicit actorRefFactory: ActorRefFactory): Future[HttpResponse] =
    apply(firstRequestTransformer = identity, sendReceive, httpRequest)

  def apply(firstRequestTransformer: RequestTransformer, sendReceive: SendReceive, request: HttpRequest)(implicit actorRefFactory: ActorRefFactory): Future[HttpResponse] = {
    import actorRefFactory.dispatcher
    val heartbeatHeader = `X-JobScheduler-Heartbeat-Start`(timing)
    val myRequest = request withHeaders heartbeatHeader :: request.headers
    (firstRequestTransformer ~> sendReceive).apply(myRequest) recover transformException flatMap handleResponse(request withEntity HttpEntity.Empty)
  }

  private def handleResponse(emptyRequest: HttpRequest)(httpResponse: HttpResponse)(implicit actorRefFactory: ActorRefFactory): Future[HttpResponse] = {
    import actorRefFactory.dispatcher
    if (!testWithHeartbeatDelay.isZero) blocking { sleep(testWithHeartbeatDelay) }
    heartbeatIdOption(httpResponse) match {
      case Some(heartbeatId) ⇒
        _heartbeatCount += 1
        val heartbeatHeader = `X-JobScheduler-Heartbeat-Continue`(heartbeatId, timing)
        val heartbeatRequest = emptyRequest withHeaders heartbeatHeader
        sendReceive.apply(heartbeatRequest) recover transformException flatMap handleResponse(emptyRequest)
      case None ⇒
        Future.successful(httpResponse)
    }
  }

  @TestOnly
  def heartbeatCount = _heartbeatCount
}

object HeartbeatRequestor {
  @ImplementedBy(classOf[StandardFactory])
  trait Factory extends (HttpHeartbeatTiming ⇒ HeartbeatRequestor)

  @Singleton
  final class StandardFactory @Inject private extends Factory {
    def apply(timing: HttpHeartbeatTiming): HeartbeatRequestor =
      new HeartbeatRequestor(timing)
  }

  private def heartbeatIdOption(httpResponse: HttpResponse): Option[HeartbeatId] =
    if (httpResponse.status == Accepted && httpResponse.entity.isEmpty)
      httpResponse.headers collectFirst { case HeartbeatResponseHeaders.`X-JobScheduler-Heartbeat`(id) ⇒ id }
    else
      None

  private def transformException[A]: PartialFunction[Throwable, A] = {
    case t: AskTimeoutException ⇒ throw new HttpRequestTimeoutException(t)
  }

  final class HttpRequestTimeoutException(cause: Throwable = null)
  extends RuntimeException("HTTP request timed out", cause)
}
