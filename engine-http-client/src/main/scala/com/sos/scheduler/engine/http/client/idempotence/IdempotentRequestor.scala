package com.sos.scheduler.engine.http.client.idempotence

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.time.timer.TimerService
import com.sos.scheduler.engine.http.client.heartbeat.HeartbeatRequestor._
import com.sos.scheduler.engine.http.client.idempotence.IdempotentHeaders.`X-JobScheduler-Request-ID`
import com.sos.scheduler.engine.http.client.idempotence.IdempotentRequestor.{DelayAfterError, LifetimeFactor, RetryTimeout, logger}
import java.time.Duration
import java.time.Instant._
import scala.concurrent.Future._
import scala.concurrent.{ExecutionContext, Future, Promise}
import scala.util.Failure
import spray.client.pipelining._
import spray.http.{HttpRequest, HttpResponse}

/**
  * @author Joacim Zschimmer
  */
final class IdempotentRequestor(requestTimeout: Duration)(implicit ec: ExecutionContext, timerService: TimerService) {
  private val newRequestId = new RequestId.Generator

  def sendAndRetry(mySendReceive: SendReceive, request: HttpRequest, requestDuration: Duration): Future[HttpResponse] = {
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
          timerService.delay(DelayAfterError, msg, cancelWhenCompleted = promise.future) onElapsed {
            failedPromise.trySuccess("After error")
          }
        case o ⇒ promise tryComplete o
      }
      timerService.at(
        now + retriedRequestDuration + RetryTimeout min timeoutAt,
        s"${request.uri} retry timeout",
        cancelWhenCompleted = firstCompletedOf(List(promise.future, response)))
        .onElapsed {
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
}

object IdempotentRequestor {
  val RetryTimeout = 1.s
  private val DelayAfterError = 1.s
  private val LifetimeFactor = 2
  private val logger = Logger(getClass)
}
