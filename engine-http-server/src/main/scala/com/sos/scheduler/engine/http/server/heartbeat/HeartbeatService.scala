package com.sos.scheduler.engine.http.server.heartbeat

import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.common.scalautil.{Logger, ScalaConcurrentHashMap}
import com.sos.scheduler.engine.common.sprayutils.Marshalling.marshalToHttpResponse
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.time.timer.TimerService
import com.sos.scheduler.engine.http.client.heartbeat.HeartbeatRequestHeaders._
import com.sos.scheduler.engine.http.client.heartbeat.{HeartbeatId, HeartbeatResponseHeaders, HttpHeartbeatTiming}
import com.sos.scheduler.engine.http.server.heartbeat.ClientSideHeartbeatService._
import com.sos.scheduler.engine.http.server.heartbeat.HeartbeatService._
import com.sos.scheduler.engine.http.server.idempotence.Idempotence
import java.time.Instant.now
import java.time.{Duration, Instant}
import java.util.concurrent.atomic.AtomicReference
import org.jetbrains.annotations.TestOnly
import scala.collection.immutable
import scala.concurrent._
import spray.http.StatusCodes.{Accepted, BadRequest}
import spray.http._
import spray.httpx.marshalling._
import spray.routing.Directives._
import spray.routing.{ExceptionHandler, Route}

/**
  * @author Joacim Zschimmer
  */
final class HeartbeatService(implicit timerService: TimerService) {

  private val idempotence = new Idempotence
  private val pendingOperations = new ScalaConcurrentHashMap[HeartbeatId, PendingOperation]
  private var unsafeStartCount = 0
  private var unsafeCount = 0
  private var unsafePendingOperationsMaximum = 0

  def startHeartbeat[A](
    onHeartbeat: Duration ⇒ Unit = _ ⇒ {},
    @Deprecated @TestOnly onHeartbeatTimeout: Option[OnHeartbeatTimeout] = None)
    (operation: Option[Duration] ⇒ Future[A])
    (implicit marshaller: Marshaller[A], actorRefFactory: ActorRefFactory): Route =
  {
    import actorRefFactory.dispatcher
    headerValueByName(`X-JobScheduler-Heartbeat-Start`.name) { case `X-JobScheduler-Heartbeat-Start`.Value(timing) ⇒
      requestUri { uri ⇒
        idempotence {
          unsafeStartCount += 1
          val responseFuture = operation(Some(timing.timeout)) map marshalToHttpResponse
          val pendingOperation = new PendingOperation(uri, responseFuture, onHeartbeat, onHeartbeatTimeout)(actorRefFactory.dispatcher)
          startHeartbeatPeriod(pendingOperation, timing)
        }
      }
    } ~
      onSuccess(operation(None)) { response ⇒ complete(response) }
  }

  def continueHeartbeat(onClientHeartbeat: Duration ⇒ Unit)(implicit actorRefFactory: ActorRefFactory): Route =
    clientSideHeartbeat(onClientHeartbeat) ~ continueHeartbeat

  def continueHeartbeat(implicit actorRefFactory: ActorRefFactory): Route =
    headerValueByName(`X-JobScheduler-Heartbeat-Continue`.name) { case `X-JobScheduler-Heartbeat-Continue`.Value(heartbeatId, times) ⇒
      handleExceptions(ExceptionHandler { case t: UnknownHeartbeatIdException ⇒ complete(BadRequest, s"Unknown or expired $heartbeatId" )}) {
        requestEntityEmpty {
          idempotence {
            val pendingOperation = pendingOperations.remove(heartbeatId) getOrElse { throw new UnknownHeartbeatIdException } // Catched above
            pendingOperation.onHeartbeat(times.timeout)
            startHeartbeatPeriod(pendingOperation, times)
          }
        }
      } ~
        complete(BadRequest, "Heartbeat with payload?")
    }

  private def startHeartbeatPeriod(pendingOperation: PendingOperation, timing: HttpHeartbeatTiming)(implicit actorRefFactory: ActorRefFactory): Future[HttpResponse] = {
    import actorRefFactory.dispatcher
    val lastHeartbeatReceivedAt = now
    if (staticSupressed)
      logger.debug("Heartbeat suppressed")
    else {
      unsafeCount += 1
      timerService.delay(
        timing.period,
        s"${pendingOperation.uri} heartbeat period",
        cancelWhenCompleted = pendingOperation.responseFuture)
        .onElapsed {
          respondWithHeartbeat()
        }
    }

    def respondWithHeartbeat(): Unit = {
      val heartbeatId = HeartbeatId.generate()
      pendingOperations.insert(heartbeatId, pendingOperation)
      unsafePendingOperationsMaximum = unsafePendingOperationsMaximum max pendingOperations.size
      val oldPromise = pendingOperation.renewPromise()
      val respondedWithHeartbeat = oldPromise trySuccess HttpResponse(Accepted, headers = HeartbeatResponseHeaders.`X-JobScheduler-Heartbeat`(heartbeatId) :: Nil)
      if (respondedWithHeartbeat) {
        startHeartbeatTimeout(heartbeatId)
      } else {
        pendingOperations -= heartbeatId
      }
    }

    def startHeartbeatTimeout(heartbeatId: HeartbeatId): Unit = {
      for (onHeartbeatTimeout ← pendingOperation.onHeartbeatTimeout) {
        timerService.delay(timing.timeout, name = s"${pendingOperation.uri} heartbeat timeout") onElapsed {
          for (o ← pendingOperations.remove(heartbeatId)) {
            logger.warn(s"No heartbeat after ${timing.period.pretty} for $pendingOperation")
            onHeartbeatTimeout(HeartbeatTimeout(heartbeatId, since = lastHeartbeatReceivedAt, timing, name = pendingOperation.uri.toString))
          }
        }
      }
    }

    pendingOperation.currentFuture
  }

  def overview = HeartbeatView(
    startCount = unsafeStartCount,
    count = unsafeCount,
    concurrentMaximum = unsafePendingOperationsMaximum,
    pendingOperations.toMap map { case (id, pendingOperation) ⇒ id → pendingOperation.toView })

  private[heartbeat] def pendingHeartbeatIds: immutable.Set[HeartbeatId] = pendingOperations.keys.toSet

  private[heartbeat] def pendingOperationsMaximum: Int = unsafePendingOperationsMaximum
}

object HeartbeatService {
  private val logger = Logger(getClass)
  @TestOnly var staticSupressed: Boolean = false

  private final class PendingOperation(
    val uri: Uri,
    val responseFuture: Future[HttpResponse],
    val onHeartbeat: Duration ⇒ Unit,
    val onHeartbeatTimeout: Option[OnHeartbeatTimeout])
    (implicit ec: ExecutionContext)
  {
    private val currentPromiseRef = new AtomicReference(Promise[HttpResponse]())
    private val startedAt = now
    private var lastAt: Option[Instant] = None
    private var heartbeatCount = 0

    responseFuture onComplete { responseTry ⇒
      val completed = currentPromiseRef.get.tryComplete(responseTry)
      if (!completed) {  // Race condition: A new heartbeat period has begun. So we complete the new promise.
        currentPromiseRef.get.complete(responseTry)
      }
    }

    def renewPromise(): Promise[HttpResponse] = {
      heartbeatCount += 1
      lastAt = Some(now)
      currentPromiseRef getAndSet Promise()
    }

    def currentFuture = currentPromiseRef.get.future

    def toView = HeartbeatView.PendingOperation(
      startedAt = PendingOperation.this.startedAt,
      lastAt = PendingOperation.this.lastAt,
      count = heartbeatCount)

    override def toString = s"PendingOperation($uri)"
  }

  type OnHeartbeatTimeout = HeartbeatTimeout ⇒ Unit

  private class UnknownHeartbeatIdException extends RuntimeException
}