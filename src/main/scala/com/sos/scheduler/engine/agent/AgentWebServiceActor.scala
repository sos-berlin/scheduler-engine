package com.sos.scheduler.engine.agent

import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.Future
import spray.http.MediaTypes._
import spray.http.{ContentType, HttpEntity}
import spray.routing.{HttpServiceActor, RequestEntityExpectedRejection, UnsupportedRequestContentTypeRejection}

/**
 * @author Joacim Zschimmer
 */
final class AgentWebServiceActor(executeCommand: String ⇒ Future[xml.Elem]) extends HttpServiceActor {

  def receive = runRoute(route)

  private def route =
    (decompressRequest() | compressResponseIfRequested(())) {
      path("jobscheduler" / "engine" / "command") {
        post {
          entity(as[HttpEntity]) {
            case HttpEntity.Empty ⇒ reject(RequestEntityExpectedRejection)
            case httpEntity: HttpEntity.NonEmpty ⇒
              httpEntity.contentType match {
                case ContentType(`application/xml` | `text/xml`, _) ⇒
                  val future = executeCommand(httpEntity.asString)
                  onSuccess(future) { response ⇒ complete(response) }
                case _ ⇒ reject(UnsupportedRequestContentTypeRejection("application/xml expected"))
              }
          }
        }
      }
    }
}
