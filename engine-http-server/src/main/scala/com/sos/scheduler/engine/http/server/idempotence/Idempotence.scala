package com.sos.scheduler.engine.http.server.idempotence

import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.alarm.{Alarm, AlarmClock}
import com.sos.scheduler.engine.http.client.idempotence.IdempotentHeaders.`X-JobScheduler-Request-ID`
import com.sos.scheduler.engine.http.client.idempotence.RequestId
import com.sos.scheduler.engine.http.server.idempotence.Idempotence._
import java.time.{Duration, Instant}
import java.util.concurrent.atomic.AtomicReference
import org.jetbrains.annotations.TestOnly
import scala.concurrent._
import spray.http.StatusCodes.BadRequest
import spray.http._
import spray.routing.Directives._
import spray.routing.Route

/**
  * @author Joacim Zschimmer
  */
final class Idempotence(implicit alarmClock: AlarmClock) {

  private val eatRequestId = new RequestId.Eater
  private val pendingOperation = new AtomicReference[Operation]

  /**
    * Registers a new idempotent request.
    * stealKnownRequest should have been called before.
    * @param body is only executed with the first request
    */
  def apply(body: ⇒ Future[HttpResponse])(implicit actorRefFactory: ActorRefFactory): Route = {
    import actorRefFactory.dispatcher
    headerValueByName(`X-JobScheduler-Request-ID`.name) { case `X-JobScheduler-Request-ID`.Value(id, lifetime) ⇒
      requestUri { uri ⇒
        complete {
          apply(id, lifetime, uri)(body)
        }
      }
    } ~ {
      // No RequestID, not idempotent
      complete { body }
    }
  }

  private def apply(id: RequestId, lifetime: Duration, uri: Uri)(body: ⇒ Future[HttpResponse])(implicit ec: ExecutionContext): Future[HttpResponse] = {
    val newPromise = Promise[HttpResponse]()
    val newOperation = Operation(id, uri, newPromise.future)
    if (eatRequestId(id)) {
      pendingOperation.getAndSet(newOperation) match {
        case null ⇒
        case oldOperation ⇒ for (alarm ← Option(oldOperation.lifetimeAlarm.get)) alarmClock.cancel(alarm)
      }
      logger.trace(s"New $uri $id")
      body onComplete newPromise.complete
      newOperation.lifetimeAlarm set alarmClock.delay(lifetime, s"$uri $id lifetime") {
        pendingOperation.compareAndSet(newOperation, null)  // Release memory of maybe big HttpResponse
      }
      newOperation.future
    } else {
      val known = pendingOperation.get
      if (known != null && known.id == id) {
        if (uri == known.uri) {
          logger.info(s"Duplicate HTTP request received for " + (if (known.future.isCompleted) "completed" else "outstanding") + s" operation $uri $id ${known.instant}")
          known.future
        } else
          Future.successful(HttpResponse(BadRequest, s"Duplicate HTTP request does not match URI"))
      } else {
        var msg: String = null
        if (id < eatRequestId.expectedId) {
          msg = s"HTTP request with expired $id is rejected"
          logger.info(msg)
        } else {
          msg = s"HTTP request with unexpected $id"
          logger.warn(s"$msg, expected now is ${eatRequestId.expectedId}")
        }
        Future.successful(HttpResponse(BadRequest, msg))
      }
    }
  }

  @TestOnly
  private[server] def pendingRequestIds: Option[RequestId] = Option(pendingOperation.get) map { _.id }
}

object Idempotence {
  private val logger = Logger(getClass)

  private final case class Operation(id: RequestId, uri: Uri, future: Future[HttpResponse]) {
    val lifetimeAlarm = new AtomicReference[Alarm[_]]
    val instant = Instant.now
  }
}
