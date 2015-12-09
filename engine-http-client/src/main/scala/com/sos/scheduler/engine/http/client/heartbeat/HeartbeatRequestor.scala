package com.sos.scheduler.engine.http.client.heartbeat

import akka.actor.ActorRefFactory
import com.google.inject.ImplementedBy
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.SideEffect.ImplicitSideEffect
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.time.timer.{Timer, TimerService}
import com.sos.scheduler.engine.http.client.heartbeat.HeartbeatRequestHeaders._
import com.sos.scheduler.engine.http.client.heartbeat.HeartbeatRequestor._
import com.sos.scheduler.engine.http.client.idempotence.IdempotentHeaders.`X-JobScheduler-Request-ID`
import com.sos.scheduler.engine.http.client.idempotence.RequestId
import java.time.Instant.now
import java.time.{Duration, Instant}
import java.util.concurrent.atomic.AtomicReference
import javax.inject.{Inject, Singleton}
import org.jetbrains.annotations.TestOnly
import scala.concurrent.Future.firstCompletedOf
import scala.concurrent.{Future, Promise, blocking}
import scala.util.{Failure, Success}
import spray.client.pipelining._
import spray.http.StatusCodes.{Accepted, OK}
import spray.http.{HttpEntity, HttpRequest, HttpResponse}

/**
  * @author Joacim Zschimmer
  *
  * @see http://kamon.io/teamblog/2014/11/02/understanding-spray-client-timeout-settings/
  */
final class HeartbeatRequestor private[http](
  timing: HttpHeartbeatTiming,
  testWithHeartbeatDelay: Duration = 0.s,
  debug: Debug = new Debug)(
  implicit timerService: TimerService,
  actorRefFactory: ActorRefFactory)
extends AutoCloseable {

  import actorRefFactory.dispatcher

  private val requestTimeout = debug.clientTimeout getOrElse timing.timeout
  private var _serverHeartbeatCount = 0
  private var _clientHeartbeatCount = 0
  private val newRequestId = new RequestId.Generator

  def close() = clientHeartbeat.cancel()  // Interrupt client heartbeat chain

  def apply(mySendReceive: SendReceive, httpRequest: HttpRequest): Future[HttpResponse] =
    apply(firstRequestTransformer = identity, mySendReceive, httpRequest)

  /**
    * @param firstRequestTransformer only used for the initial request, not used for heartbeat acknowledges
    * @param mySendReceive used for initial request and for heartbeat acknowledges
    * @param request the initial request. Without payload it is used for heartbeat
    * @return the response for the initial request
    */
  private def apply(firstRequestTransformer: RequestTransformer, mySendReceive: SendReceive, request: HttpRequest): Future[HttpResponse] = {
    clientHeartbeat.cancel()
    val emptyRequest = request withEntity HttpEntity.Empty
    val sentAt = now
    sendAndRetry(
      firstRequestTransformer ~> mySendReceive,
      request withHeaders `X-JobScheduler-Heartbeat-Start`(timing) :: request.headers,
      requestDuration = timing.period)
    .flatMap(handleResponse(mySendReceive, emptyRequest))
    .sideEffect { _ onSuccess { case _ ⇒ clientHeartbeat(sentAt, mySendReceive, emptyRequest) }}
  }

  private def handleResponse(mySendReceive: SendReceive, emptyRequest: HttpRequest)(httpResponse: HttpResponse): Future[HttpResponse] = {
    if (!testWithHeartbeatDelay.isZero) blocking { sleep(testWithHeartbeatDelay) }
    heartbeatIdOption(httpResponse) match {
      case Some(heartbeatId) ⇒
        _serverHeartbeatCount += 1
        val heartbeatRequest = emptyRequest withHeaders `X-JobScheduler-Heartbeat-Continue`(heartbeatId, timing)
        sendAndRetry(mySendReceive, heartbeatRequest, requestDuration = timing.period) flatMap handleResponse(mySendReceive, emptyRequest)
      case None ⇒
        Future.successful(httpResponse)
    }
  }

  private object clientHeartbeat {
    private val currentClientTimeout = new AtomicReference[Timer[Unit]]
    var throwableOption: Option[Throwable] = None // How to inform C++ JobScheduler about an error ???

    def cancel(): Unit =
      for (o ← Option(currentClientTimeout.get)) {
        timerService.cancel(o)
        currentClientTimeout.compareAndSet(o, null)
      }

    def apply(lastRequestSentAt: Instant, mySendReceive: SendReceive, emptyRequest: HttpRequest): Unit = {
      val timeoutAt = lastRequestSentAt + timing.period max now + ClientHeartbeatMinimumDelay
      currentClientTimeout set timerService.at(timeoutAt, name = s"${emptyRequest.uri} client-side heartbeat") {
        val heartbeatRequest = emptyRequest withHeaders `X-JobScheduler-Heartbeat`(timing)
        if (debug.suppressed) logger.debug(s"suppressed $heartbeatRequest")
        else {
          _clientHeartbeatCount += 1
          val sentAt = now
          val promise = Promise[Either[Unit, HttpResponse]]()
          val responded = mySendReceive(heartbeatRequest)
          responded map Right.apply onComplete promise.tryComplete
          timerService.delay(RetryTimeout, cancelWhenCompleted = responded, s"${heartbeatRequest.uri} client heartbeat retry timeout") {
            promise trySuccess Left(())
          }
          promise.future map {
            case Left(()) ⇒ // timeout
            case Right(HttpResponse(OK, HttpEntity.Empty, _, _)) ⇒
            case Right(HttpResponse(OK, entity, _, _)) ⇒ sys.error(s"Unexpected heartbeat payload: $entity")
            case Right(HttpResponse(status, entity, _, _)) ⇒ sys.error(s"Unexpected heartbeat response: $status" + (if (status.isFailure) s": ${entity.asString take 500}" else ""))
          } onComplete {
            case Success(()) ⇒ apply(sentAt, mySendReceive, emptyRequest)
            case Failure(t) ⇒ onClientHeartbeatTimeout(t)
          }
        }
      }
    }

    def onClientHeartbeatTimeout(throwable: Throwable): Unit =  {
      logger.error(s"$throwable")
      throwableOption = Some(throwable)
    }
  }

  private def sendAndRetry(mySendReceive: SendReceive, request: HttpRequest, requestDuration: Duration): Future[HttpResponse] = {
    val requestId = newRequestId()
    val myRequest = request withHeaders `X-JobScheduler-Request-ID`(requestId, requestTimeout * LifetimeFactor) :: request.headers
    sendAndRetry(mySendReceive, myRequest, requestId, requestDuration)
  }

  private def sendAndRetry(mySendReceive: SendReceive, request: HttpRequest, requestId: RequestId, requestDuration: Duration): Future[HttpResponse] = {
    val firstSentAt = now
    val timeoutAt = firstSentAt + requestTimeout
    val promise = Promise[HttpResponse]()

    def cycle(retryNr: Int, retriedRequestDuration: Duration): Unit = {
      val failedPromise = Promise[String]()
      //logger.debug(s"${request.method} ${request.uri} $requestId")
      val response = mySendReceive(request)
      response onComplete {
        case Failure(t) if now + DelayAfterError < timeoutAt ⇒
          val msg = s"${request.method} ${request.uri} $t"
          logger.warn(msg)
          timerService.at(now + DelayAfterError, cancelWhenCompleted = promise.future, msg) {
            failedPromise.trySuccess("After error")
          }
        case o ⇒ promise tryComplete o
      }
      timerService.at(now + retriedRequestDuration + RetryTimeout min timeoutAt,
        cancelWhenCompleted = firstCompletedOf(List(promise.future, response)),
        s"${request.uri} retry timeout")
      {
        failedPromise trySuccess s"After ${(now - firstSentAt).pretty} of no response"
      }
      failedPromise.future onSuccess { case startOfMessage ⇒
        if (!promise.isCompleted) {
          if (now < timeoutAt) {
            logger.warn(s"$startOfMessage, HTTP request of $firstSentAt is being repeated #${retryNr+1}: ${request.method} ${request.uri} $requestId")
            if (retryNr == 0) promise.future onSuccess { case _ ⇒ logger.info(s"HTTP request has finally succeeded: ${request.method} ${request.uri} $requestId") }
            cycle(retryNr + 1, retriedRequestDuration = 0.s)
          } else
            promise tryFailure new HttpRequestTimeoutException(requestTimeout)
        }
      }
    }

    cycle(retryNr = 0, retriedRequestDuration = requestDuration)
    promise.future
  }

  @TestOnly
  def serverHeartbeatCount = _serverHeartbeatCount

  @TestOnly
  def clientHeartbeatCount = _clientHeartbeatCount

  override def toString = clientHeartbeat.throwableOption mkString ("HeartbeatRequestor(", "", ")")
}

object HeartbeatRequestor {
  private val logger = Logger(getClass)
  private[http] val ClientHeartbeatMinimumDelay = 1.s  // Client-side heartbeat is sent after this delay after last response without new regular request
  private val RetryTimeout = 1.s
  private val DelayAfterError = 1.s
  private val LifetimeFactor = 2

  @ImplementedBy(classOf[StandardFactory])
  trait Factory extends (HttpHeartbeatTiming ⇒ HeartbeatRequestor)

  @Singleton
  final class StandardFactory @Inject private(implicit timerService: TimerService, actorRefFactory: ActorRefFactory, debug: Debug) extends Factory {
    def apply(timing: HttpHeartbeatTiming): HeartbeatRequestor = new HeartbeatRequestor(timing, debug = debug)
  }

  private def heartbeatIdOption(httpResponse: HttpResponse): Option[HeartbeatId] =
    if (httpResponse.status == Accepted && httpResponse.entity.isEmpty)
      httpResponse.headers collectFirst { case HeartbeatResponseHeaders.`X-JobScheduler-Heartbeat`(id) ⇒ id }
    else
      None

  final class HttpRequestTimeoutException(timeout: Duration) extends RuntimeException(s"HTTP request timed out due to http-heartbeat-timeout=${timeout.pretty}")

  @Singleton final class Debug @Inject() () {
    var clientTimeout: Option[Duration] = None
    var suppressed = false
  }
}
