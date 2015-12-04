package com.sos.scheduler.engine.http.server.idempotence

import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.common.scalautil.{Logger, ScalaConcurrentHashMap}
import com.sos.scheduler.engine.common.time.alarm.AlarmClock
import com.sos.scheduler.engine.http.client.idempotence.IdempotentHeaders.`X-JobScheduler-Request-ID`
import com.sos.scheduler.engine.http.client.idempotence.RequestId
import com.sos.scheduler.engine.http.server.idempotence.Idempotence._
import java.time.Duration
import javax.inject.{Inject, Singleton}
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
@Singleton
final class Idempotence @Inject() (alarmClock: AlarmClock) {

  private val operations = new ScalaConcurrentHashMap[RequestId, Operation]()

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

  def apply(id: RequestId, lifetime: Duration, uri: Uri)(body: ⇒ Future[HttpResponse])(implicit ec: ExecutionContext): Future[HttpResponse] = {
    val promise = Promise[HttpResponse]()
    val newOperation = Operation(uri, promise.future)
    operations.delegate.putIfAbsent(id, newOperation) match {
      case null ⇒
        val future = body // Execute only if request is new body
        future onComplete promise.complete
        alarmClock.delay(lifetime) {operations -= id}
        newOperation.responseFuture
      case Operation(`uri`, knownFuture) ⇒
        logger.debug(s"Duplicate HTTP request received for " + (if (knownFuture.isCompleted) "completed" else "outstanding") + s" operation $uri $id")
        knownFuture
      case knownOperation ⇒
        Future.successful(HttpResponse(BadRequest, s"Duplicate HTTP request does not match URI"))
    }
  }

  @TestOnly
  private[server] def pendingRequestIds: immutable.Set[RequestId] = operations.keys.toSet
}

object Idempotence {
  private val logger = Logger(getClass)
  private final case class Operation(uri: Uri, responseFuture: Future[HttpResponse])
}
