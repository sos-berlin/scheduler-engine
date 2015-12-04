package com.sos.scheduler.engine.http.client.heartbeat

import akka.actor.ActorRefFactory
import com.google.inject.ImplementedBy
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.SwitchOnAtomicBoolean
import com.sos.scheduler.engine.common.scalautil.SideEffect.ImplicitSideEffect
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.time.alarm.AlarmClock
import com.sos.scheduler.engine.http.client.heartbeat.HeartbeatRequestHeaders._
import com.sos.scheduler.engine.http.client.heartbeat.HeartbeatRequestor._
import com.sos.scheduler.engine.http.client.idempotence.IdempotentHeaders.`X-JobScheduler-Request-ID`
import com.sos.scheduler.engine.http.client.idempotence.RequestId
import java.time.Duration
import java.time.Instant.now
import java.util.concurrent.atomic.{AtomicBoolean, AtomicInteger}
import javax.inject.{Inject, Singleton}
import org.jetbrains.annotations.TestOnly
import scala.concurrent.{Future, Promise, blocking}
import scala.util.{Failure, Success}
import spray.client.pipelining._
import spray.http.StatusCodes.{Accepted, OK}
import spray.http.{HttpHeader, HttpEntity, HttpRequest, HttpResponse}

/**
  * @author Joacim Zschimmer
  *
  * @see http://kamon.io/teamblog/2014/11/02/understanding-spray-client-timeout-settings/
  */
final class HeartbeatRequestor private[http](
  timing: HttpHeartbeatTiming,
  testWithHeartbeatDelay: Duration = 0.s,
  debug: Debug = new Debug)(
  implicit alarmClock: AlarmClock,
  actorRefFactory: ActorRefFactory)
extends AutoCloseable {

  import actorRefFactory.dispatcher

  private val requestTimeout = debug.clientTimeout getOrElse timing.timeout
  private val requestRetryAfter = timing.period * HeartbeatPeriodRetryFactor
  private val requestLifetime = requestTimeout * LifetimeFactor
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
    val emptyRequest = request withEntity HttpEntity.Empty
    val nr = requestCounter.incrementAndGet()
    sendAndRetry(
      firstRequestTransformer ~> mySendReceive,
      request withHeaders `X-JobScheduler-Heartbeat-Start`(timing) :: request.headers)
    .flatMap(handleResponse(mySendReceive, emptyRequest))
    .sideEffect { _ onSuccess { case _ ⇒ doClientHeartbeat(nr, mySendReceive, emptyRequest) }}
  }

  private def handleResponse(mySendReceive: SendReceive, emptyRequest: HttpRequest)(httpResponse: HttpResponse): Future[HttpResponse] = {
    if (!testWithHeartbeatDelay.isZero) blocking { sleep(testWithHeartbeatDelay) }
    heartbeatIdOption(httpResponse) match {
      case Some(heartbeatId) ⇒
        _serverHeartbeatCount += 1
        val heartbeatRequest = emptyRequest withHeaders `X-JobScheduler-Heartbeat-Continue`(heartbeatId, timing)
        sendAndRetry(mySendReceive, heartbeatRequest) flatMap handleResponse(mySendReceive, emptyRequest)
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
          val responseFuture = sendAndRetry(mySendReceive, heartbeatRequest)  // Send with idempotence.RequestId, but server ignores this ID
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

  private def sendAndRetry(mySendReceive: SendReceive, request: HttpRequest): Future[HttpResponse] = {
    val requestId = RequestId.generate()
    val myRequest = request withHeaders `X-JobScheduler-Request-ID`(requestId, lifetime = requestLifetime) :: request.headers
    sendAndRetry(mySendReceive, myRequest, requestId)
  }

  private def sendAndRetry(mySendReceive: SendReceive, request: HttpRequest, requestId: RequestId): Future[HttpResponse] = {
    val timeoutAt = now + requestTimeout
    val promise = Promise[HttpResponse]()
    def cycle(): Unit = {
      val responseFuture = mySendReceive(request)
      val sentAt = now
      val retried = new AtomicBoolean
      def retry(text: String): Unit = {
        if (!promise.isCompleted) {
          if (now < timeoutAt) {
            retried.switchOn {
              logger.warn(s"$text, HTTP request of $sentAt is being repeated: ${request.method} ${request.uri} $requestId")
              cycle()
            }
          }
          else promise tryFailure new HttpRequestTimeoutException(requestTimeout)
        }
      }
      responseFuture onComplete {
        case Failure(t) if now < timeoutAt ⇒
          logger.warn(s"${request.method} ${request.uri} $t")
          alarmClock.delay(DelayAfterError) {
            retry("After error")
          }
        case o ⇒ promise tryComplete o
      }
      alarmClock.at(sentAt + requestRetryAfter min timeoutAt, s"${request.uri} retry timeout") {
        retry("After no response")
      }
    }
    cycle()
    promise.future
  }

  @TestOnly
  def serverHeartbeatCount = _serverHeartbeatCount

  @TestOnly
  def clientHeartbeatCount = _clientHeartbeatCount

  override def toString = Option(clientHeartbeatThrowable) mkString ("HeartbeatRequestor(", "", ")")
}

object HeartbeatRequestor {
  private val logger = Logger(getClass)
  private val HeartbeatPeriodRetryFactor = BigDecimal("1.2")
  private val LifetimeFactor = BigDecimal("1.5")
  private val DelayAfterError = 1.s

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

  private trait TimedRequest {
    def request: HttpRequest
    def timing: HttpHeartbeatTiming
    protected def header: HttpHeader
    def requestWithHeaders = request withHeaders header :: request.headers
  }

  private case class ATimedRequest(request: HttpRequest, heartbeatId: HeartbeatId, timing: HttpHeartbeatTiming) extends TimedRequest {
    def header = `X-JobScheduler-Heartbeat-Continue`(heartbeatId, timing)
    // Der wird sowieso sofort beantwortet
  }

  private case class BTimedRequest(request: HttpRequest, timing: HttpHeartbeatTiming) extends TimedRequest {
    def header = `X-JobScheduler-Heartbeat`(timing)
    // Wollen wir die gleiche Anfrage (RequestID) mit anderem Header schicken?
  }

  final class HttpRequestTimeoutException(timeout: Duration) extends RuntimeException(s"HTTP request timed out due to http-heartbeat-timeout=${timeout.pretty}")

  @Singleton final class Debug @Inject() () {
    var clientTimeout: Option[Duration] = None
    var suppressed = false
  }
}
