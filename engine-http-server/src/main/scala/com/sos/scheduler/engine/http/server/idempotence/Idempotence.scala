package com.sos.scheduler.engine.http.server.idempotence

import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.common.scalautil.{Logger, ScalaConcurrentHashMap}
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.time.alarm.AlarmClock
import com.sos.scheduler.engine.http.client.idempotence.IdempotentHeaders.`X-JobScheduler-Request-ID`
import com.sos.scheduler.engine.http.client.idempotence.RequestId
import com.sos.scheduler.engine.http.server.idempotence.Idempotence._
import java.time.{Duration, Instant}
import org.jetbrains.annotations.TestOnly
import scala.collection.immutable
import scala.concurrent._
import spray.http.StatusCodes.BadRequest
import spray.http._
import spray.routing.Directives._
import spray.routing.Route

/**
  * @author Joacim Zschimmer
  */
final class Idempotence(implicit alarmClock: AlarmClock) {

  private val operations = new ScalaConcurrentHashMap[RequestId, Operation]()
  private val eatRequestId = new RequestId.Eater

  /**
    * Registers a new idempotent request.
    * stealKnownRequest should have been called before.
    * @param body is only executed with the first request
    */
  def apply(body: ⇒ Future[HttpResponse])(implicit actorRefFactory: ActorRefFactory): Route = {
    import actorRefFactory.dispatcher
    headerValueByName(`X-JobScheduler-Request-ID`.name) { case `X-JobScheduler-Request-ID`.Value(id, timeout) ⇒
      requestUri { uri ⇒
        complete {
          apply(id, timeout, uri)(body)
        }
      }
    } ~ {
      // No RequestID, not idempotent
      complete { body }
    }
  }

  private def apply(id: RequestId, timeout: Duration, uri: Uri)(body: ⇒ Future[HttpResponse])(implicit ec: ExecutionContext): Future[HttpResponse] = {
    val newPromise = Promise[HttpResponse]()
    val newOperation = Operation(uri, newPromise.future)
    if (eatRequestId(id))
      operations.delegate.putIfAbsent(id, newOperation) match {
        case null ⇒
          logger.debug(s"New $uri $id")
          body onComplete newPromise.complete
          alarmClock.delay(timeout + (LifetimeExtra min timeout), s"$uri $id lifetime") {
            operations -= id
          }
          newOperation.future
        case someOperation ⇒
          val msg = s"$uri New but registered $id ???"
          logger.error(msg)
          Future.failed(new RuntimeException(msg))
      }
    else
      operations.get(id) match {
        case Some(knownOperation: Operation) ⇒
          if (uri != knownOperation.uri)
            Future.successful(HttpResponse(BadRequest, s"Duplicate HTTP request does not match URI"))
          else {
            logger.debug(s"Duplicate HTTP request received for " + (if (knownOperation.future.isCompleted) "completed" else "outstanding") + s" operation $uri $id ${knownOperation.instant}")
            knownOperation.future
          }
        case None ⇒
          val msg = s"Expected (possibly) ${eatRequestId.expectedId} instead of $id"
          logger.warn(s"$uri $msg")
          Future.successful(HttpResponse(BadRequest, msg))
      }
  }

  @TestOnly
  private[server] def pendingRequestIds: immutable.Set[RequestId] = operations.keys.toSet
}

object Idempotence {
  private val logger = Logger(getClass)
  private val LifetimeExtra = 5.s

  private final case class Operation(uri: Uri, future: Future[HttpResponse]) {
    val instant = Instant.now
  }
}
