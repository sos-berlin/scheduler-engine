package com.sos.scheduler.engine.plugins.newwebservice.routes

import akka.actor.ActorRefFactory
import com.sos.scheduler.engine.agent.client.AgentClient
import com.sos.scheduler.engine.common.sprayutils.SprayJsonOrYamlSupport._
import com.sos.scheduler.engine.plugins.newwebservice.routes.AgentRoute._
import scala.collection.immutable
import scala.concurrent.ExecutionContext
import scala.util.{Failure, Success}
import spray.client.pipelining._
import spray.http.StatusCodes.{BadRequest, Forbidden}
import spray.http.{HttpResponse, Uri}
import spray.httpx.UnsuccessfulResponseException
import spray.json.DefaultJsonProtocol._
import spray.json.{JsNumber, JsObject, JsString}
import spray.routing.Directives._
import spray.routing.{Route, ValidationRejection}


/**
  * @author Joacim Zschimmer
  */
trait AgentRoute {
  protected implicit def actorRefFactory: ActorRefFactory
  protected implicit def executionContext: ExecutionContext
  protected def agentClients: immutable.Seq[AgentClient]

  def agentRoute: Route =
    get {
      rawPathPrefix(Slash) {
        extract(identity) { requestContext ⇒
          // unmatchedPath and query contain a full Agent URI
          val (agentUri, tailUri) = splitIntoAgentUriAndTail(requestContext.unmatchedPath, requestContext.request.uri.query)
          if (!isAllowedUri(tailUri))
            complete(Forbidden)
          else {
            agentClients find { _.agentUri == agentUri } match {
              case None ⇒
                reject(ValidationRejection("Unknown Agent"))
              case Some(agentClient) ⇒
                onComplete(agentClient[HttpResponse](Get(tailUri))) {
                  case Success(result) ⇒
                    complete(result)
                  case Failure(e: UnsuccessfulResponseException) ⇒
                    complete((BadRequest, JsObject(
                      "status" → JsNumber(e.response.status.intValue),
                      "message" → JsString(e.getMessage))))
                  case Failure(throwable) ⇒
                    complete((BadRequest, JsObject(
                      "message" → JsString(throwable.toString))))
                }
            }
          }
        }
      }
    }
}

object AgentRoute {
  private def splitIntoAgentUriAndTail(unmatchedPath: Uri.Path, query: Uri.Query): (Uri, Uri) = {
    val agentRequestUri: Uri = Uri(unmatchedPath.toString)
    val Uri(scheme, authority, path, Uri.Query.Empty, None) = agentRequestUri
    (Uri(scheme, authority), Uri(path = path, query = query))
  }

  private def isAllowedUri(uri: Uri) =
    isAllowedPath(uri.path) && uri.query.isEmpty && uri.fragment.isEmpty

  private def isAllowedPath = Set(Uri.Path("/jobscheduler/agent/api"))
}
