package com.sos.scheduler.engine.http.server.idempotence

import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.common.scalautil.{Logger, ScalaConcurrentHashMap}
import com.sos.scheduler.engine.common.time.alarm.AlarmClock
import com.sos.scheduler.engine.http.client.idempotence.IdempotentHeaders.`X-JobScheduler-Request-ID`
import com.sos.scheduler.engine.http.client.idempotence.RequestId
import com.sos.scheduler.engine.http.server.idempotence.Idempotence._
import java.time.{Duration, Instant}
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

  private def apply(id: RequestId, lifetime: Duration, uri: Uri)(body: ⇒ Future[HttpResponse])(implicit ec: ExecutionContext): Future[HttpResponse] = {
    val newPromise = Promise[HttpResponse]()
    val newOperation = Operation(uri, newPromise.future)
    operations.delegate.putIfAbsent(id, newOperation) match {
      case null ⇒
        logger.debug(s"New $uri $id")
        body onComplete newPromise.complete
        alarmClock.delay(lifetime, s"$uri $id lifetime") { operations -= id }
        newOperation.future
      case knownOperation: Operation ⇒
        if (uri != knownOperation.uri)
          Future.successful(HttpResponse(BadRequest, s"Duplicate HTTP request does not match URI"))
        else {
          logger.debug(s"Duplicate HTTP request received for " + (if (knownOperation.future.isCompleted) "completed" else "outstanding") + s" operation $uri $id ${knownOperation.instant}")
          knownOperation.future
        }
    }
  }

  @TestOnly
  private[server] def pendingRequestIds: immutable.Set[RequestId] = operations.keys.toSet
}

object Idempotence {
  private val logger = Logger(getClass)

  private final case class Operation(uri: Uri, future: Future[HttpResponse]) {
    val instant = Instant.now
  }
}
