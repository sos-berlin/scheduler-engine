package com.sos.scheduler.engine.http.server.heartbeat

import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.common.scalautil.{Logger, ScalaConcurrentHashMap}
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.http.client.heartbeat.HeartbeatHeaders._
import com.sos.scheduler.engine.http.client.heartbeat.{HeartbeatId, HttpHeartbeatTiming}
import com.sos.scheduler.engine.http.server.heartbeat.HeartbeatService._
import java.time.Instant
import java.util.concurrent.atomic.AtomicReference
import scala.collection.immutable
import scala.concurrent._
import scala.math.max
import spray.http.StatusCodes.{Accepted, BadRequest, OK}
import spray.http._
import spray.httpx.marshalling._
import spray.routing.Directives._
import spray.routing.Route

/**
  * @author Joacim Zschimmer
  */
final class HeartbeatService {

  private val pendingOperations = new ScalaConcurrentHashMap[HeartbeatId, PendingOperation]()
  private var _pendingOperationsMaximum = 0  // Possibly not really thread-safe

  def startHeartbeat[A](
    resultFuture: Future[A],
    onHeartbeatTimeout: OnHeartbeatTimeout = defaultOnClientTimeout)
    (implicit actorRefFactory: ActorRefFactory, marshaller: Marshaller[A]): Route =
  {
    import actorRefFactory.dispatcher
    val responseFuture = resultFuture map { result ⇒
      marshalToEntityAndHeaders(result) match {
        case Left(t) ⇒ throw t
        case Right((entity, headers)) ⇒ HttpResponse(OK, entity, headers.toList)
      }
    }
    headerValueByName(`X-JobScheduler-Heartbeat-Start`.name) { case `X-JobScheduler-Heartbeat-Start`.Value(timing) ⇒
      requestUri { uri ⇒
        val pendingOperation = new PendingOperation(uri, responseFuture, onHeartbeatTimeout)(actorRefFactory.dispatcher)
        startHeartbeatPeriod(pendingOperation, timing)
      }
    } ~
      onSuccess(responseFuture) { response ⇒ complete(response) }
  }

  def continueHeartbeat(implicit actorRefFactory: ActorRefFactory): Route =
    headerValueByName(`X-JobScheduler-Heartbeat-Continue`.name) {
     case `X-JobScheduler-Heartbeat-Continue`.Value(heartbeatId, times) ⇒
        requestEntityEmpty {
          pendingOperations.remove(heartbeatId) match {
            case None ⇒ complete(BadRequest, "Unknown heartbeat ID (HTTP request is too late?)")
            case Some(o) ⇒ startHeartbeatPeriod(o, times)
          }
        } ~
          complete(BadRequest, "Heartbeat with entity?")
    }

  private def startHeartbeatPeriod(pendingOperation: PendingOperation, timing: HttpHeartbeatTiming)(implicit actorRefFactory: ActorRefFactory): Route = {
    import actorRefFactory.dispatcher
    val lastHeartbeatReceivedAt = Instant.now()
    Future {
      blocking { sleep(timing.period) }
      val heartbeatId = HeartbeatId.generate()
      pendingOperations.insert(heartbeatId, pendingOperation)
      _pendingOperationsMaximum = max(_pendingOperationsMaximum, pendingOperations.size)
      val oldPromise = pendingOperation.renewPromise()
      val heartbeatResponded = oldPromise trySuccess HttpResponse(Accepted, headers = `X-JobScheduler-Heartbeat`(heartbeatId) :: Nil)
      if (heartbeatResponded) {
        blocking { sleep(timing.timeout) }
        for (o ← pendingOperations.remove(heartbeatId)) {
          logger.debug(s"No heartbeat after ${timing.period.pretty} for $pendingOperation")
          o.onHeartbeatTimeout(HeartbeatTimeout(heartbeatId, since = lastHeartbeatReceivedAt, timing, name = pendingOperation.uri.toString))
        }
      } else {
        pendingOperations -= heartbeatId
      }
    }
    onSuccess(pendingOperation.currentFuture) { response ⇒ complete(response) }
  }

  private[heartbeat] def pendingHeartbeatIds: immutable.Set[HeartbeatId] = pendingOperations.keys.toSet

  private[heartbeat] def pendingOperationsMaximum: Int = _pendingOperationsMaximum
}

object HeartbeatService {
  private val logger = Logger(getClass)

  private final class PendingOperation(
    val uri: Uri,
    responseFuture: Future[HttpResponse],
    val onHeartbeatTimeout: OnHeartbeatTimeout)
    (implicit ec: ExecutionContext)
  {
    private val currentPromiseRef = new AtomicReference(Promise[HttpResponse]())

    responseFuture onComplete { responseTry ⇒
      val completed = currentPromiseRef.get.tryComplete(responseTry)
      if (!completed) {  // Race condition: A new heartbeat period has begun. So we complete the new promise.
        currentPromiseRef.get.complete(responseTry)
      }
    }

    def renewPromise(): Promise[HttpResponse] = currentPromiseRef getAndSet Promise()

    def currentFuture = currentPromiseRef.get.future

    override def toString = s"PendingOperation($uri)"
  }

  private def defaultOnClientTimeout(timedOut: HeartbeatTimeout): Unit = {
    import timedOut.{name, timing}
    logger.warn(s"Expected heartbeating HTTP request has not arrived in time ${timing.timeout.pretty} for $name")
  }

  type OnHeartbeatTimeout = HeartbeatTimeout ⇒ Unit
}
