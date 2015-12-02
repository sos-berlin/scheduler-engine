package com.sos.scheduler.engine.http.client.heartbeat

import akka.actor.ActorRefFactory
import akka.pattern.AskTimeoutException
import com.google.inject.ImplementedBy
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.time.alarm.AlarmClock
import com.sos.scheduler.engine.http.client.heartbeat.HeartbeatRequestHeaders._
import com.sos.scheduler.engine.http.client.heartbeat.HeartbeatRequestor._
import java.time.Duration
import java.util.concurrent.atomic.AtomicInteger
import javax.inject.{Inject, Singleton}
import org.jetbrains.annotations.TestOnly
import scala.concurrent.{Future, blocking}
import scala.util.{Failure, Success}
import spray.client.pipelining._
import spray.http.StatusCodes.{Accepted, OK}
import spray.http.{HttpEntity, HttpRequest, HttpResponse}

/**
  * @author Joacim Zschimmer
  */
final class HeartbeatRequestor private[http](
  timing: HttpHeartbeatTiming,
  testWithHeartbeatDelay: Duration = 0.s,
  debug: Debug = new Debug)(
  implicit alarmClock: AlarmClock,
  actorRefFactory: ActorRefFactory)
extends AutoCloseable {

  import actorRefFactory.dispatcher

  private var _serverHeartbeatCount = 0
  private var _clientHeartbeatCount = 0
  private val requestCounter = new AtomicInteger
  private var clientHeartbeatThrowable: Throwable = null  // How to inform C++ JobScheduler about an error ???

  def close() = requestCounter.incrementAndGet()  // Interrupt heartbeat chain

  def apply(mySendReceive: SendReceive, httpRequest: HttpRequest): Future[HttpResponse] =
    apply(firstRequestTransformer = identity, mySendReceive, httpRequest)

  /**
    * @param firstRequestTransformer only used for the initial request, not used for heartbeat acknowledges
    * @param mySendReceive used for initial request and for heartbeat acknowledges
    * @param request the initial request. Without payload it is used for heartbeat
    * @return the response for the initial request
    */
  private def apply(firstRequestTransformer: RequestTransformer, mySendReceive: SendReceive, request: HttpRequest): Future[HttpResponse] = {
    val heartbeatHeader = `X-JobScheduler-Heartbeat-Start`(timing)
    val myRequest = request withHeaders heartbeatHeader :: request.headers
    val emptyRequest = request withEntity HttpEntity.Empty
    val nr = requestCounter.incrementAndGet()
    val responseFuture = (firstRequestTransformer ~> mySendReceive).apply(myRequest) recover transformException flatMap handleResponse(mySendReceive, emptyRequest)
    responseFuture onSuccess { case _ ⇒ doClientHeartbeat(nr, mySendReceive, emptyRequest) }
    responseFuture
  }

  private def handleResponse(mySendReceive: SendReceive, emptyRequest: HttpRequest)(httpResponse: HttpResponse): Future[HttpResponse] = {
    if (!testWithHeartbeatDelay.isZero) blocking { sleep(testWithHeartbeatDelay) }
    heartbeatIdOption(httpResponse) match {
      case Some(heartbeatId) ⇒
        _serverHeartbeatCount += 1
        val heartbeatRequest = emptyRequest withHeaders `X-JobScheduler-Heartbeat-Continue`(heartbeatId, timing)
        mySendReceive.apply(heartbeatRequest) recover transformException flatMap handleResponse(mySendReceive, emptyRequest)
      case None ⇒
        Future.successful(httpResponse)
    }
  }

  private def doClientHeartbeat(nr: Int, mySendReceive: SendReceive, emptyRequest: HttpRequest): Unit = {
    alarmClock.delay(timing.period, name = s"${emptyRequest.uri} client-side heartbeat") {
      if (nr == requestCounter.get) {  // No HTTP request active and not closed? Then we send a client-side heartbeat to notify the server
        val heartbeatRequest = emptyRequest withHeaders `X-JobScheduler-Heartbeat`(timing)
        if (debug.suppressed) logger.debug(s"suppressed $heartbeatRequest")
        else {
          val responseFuture = mySendReceive(heartbeatRequest)
          _clientHeartbeatCount += 1
          responseFuture map {
            case HttpResponse(OK, HttpEntity.Empty, _, _) ⇒
            case HttpResponse(OK, entity, _, _) ⇒ sys.error(s"Unexpected heartbeat payload: $entity")
            case HttpResponse(status, entity, _, _) ⇒ sys.error(s"Unexpected heartbeat response: $status" + (if (status.isFailure) s": ${entity.asString take 500}" else ""))
          } onComplete {
            case Success(()) ⇒ doClientHeartbeat(nr, mySendReceive, emptyRequest)
            case Failure(t) ⇒
              logger.error(s"$t")
              clientHeartbeatThrowable = t
          }
        }
      }
    }
  }

  @TestOnly
  def serverHeartbeatCount = _serverHeartbeatCount

  @TestOnly
  def clientHeartbeatCount = _clientHeartbeatCount

  override def toString = Option(clientHeartbeatThrowable) mkString ("HeartbeatRequestor(", "", ")")
}

object HeartbeatRequestor {
  private val logger = Logger(getClass)

  @ImplementedBy(classOf[StandardFactory])
  trait Factory extends (HttpHeartbeatTiming ⇒ HeartbeatRequestor)

  @Singleton
  final class StandardFactory @Inject private(implicit alarmClock: AlarmClock, actorRefFactory: ActorRefFactory, debug: Debug) extends Factory {
    def apply(timing: HttpHeartbeatTiming): HeartbeatRequestor = new HeartbeatRequestor(timing, debug = debug)
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

  @Singleton final class Debug @Inject() () {
    var suppressed = false
  }
}
