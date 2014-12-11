package com.sos.scheduler.agent

import spray.http.MediaTypes._
import spray.http.{ContentType, HttpEntity}
import spray.routing.{HttpServiceActor, RequestEntityExpectedRejection, UnsupportedRequestContentTypeRejection}

/**
 * @author Joacim Zschimmer
 */
final class AgentWebServiceActor(executeCommand: String ⇒ String) extends HttpServiceActor {

  def receive = runRoute(route)

  private def route =
    (decompressRequest() | compressResponseIfRequested(())) {
      path("jobscheduler" / "engine" / "command") {
        post {
          entity(as[HttpEntity]) {
            case httpEntity: HttpEntity.NonEmpty ⇒
              httpEntity.contentType match {
                case ContentType(`application/xml` | `text/xml`, _) ⇒
                  respondWithMediaType(`text/xml`) {  // ???
                    detach(()) {
                      complete {
                        executeCommand(httpEntity.asString)
                      }
                    }
                  }
                case _ ⇒ reject(UnsupportedRequestContentTypeRejection("application/xml expected"))
              }
            case HttpEntity.Empty ⇒ reject(RequestEntityExpectedRejection)
          }
        }
      }
    }
}
