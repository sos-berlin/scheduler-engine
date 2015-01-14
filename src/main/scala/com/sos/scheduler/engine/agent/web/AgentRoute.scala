package com.sos.scheduler.engine.agent.web

import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.Future
import spray.http.HttpEntity
import spray.http.MediaTypes._
import spray.routing.{HttpService, RequestEntityExpectedRejection, UnsupportedRequestContentTypeRejection}

/**
 * @author Joacim Zschimmer
 */
private[agent] trait AgentRoute
extends HttpService {

  protected def executeCommand(command: String): Future[xml.Elem]

  private[agent] def route =
    (decompressRequest() | compressResponseIfRequested(())) {
      path("jobscheduler" / "engine" / "command") {
        post {
          entity(as[HttpEntity]) {
            case HttpEntity.Empty ⇒ reject(RequestEntityExpectedRejection)
            case httpEntity: HttpEntity.NonEmpty ⇒
              if (!(Set(`application/xml`, `text/xml`) contains httpEntity.contentType.mediaType)) reject(UnsupportedRequestContentTypeRejection("application/xml expected"))
              onSuccess(executeCommand(httpEntity.asString)) {
                response ⇒ complete(response)
              }
          }
        }
      }
    }
}
