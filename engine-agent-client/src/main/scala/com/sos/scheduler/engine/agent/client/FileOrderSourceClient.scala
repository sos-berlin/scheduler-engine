package com.sos.scheduler.engine.agent.client

import akka.actor.ActorSystem
import com.sos.scheduler.engine.agent.data.{FileOrderSourceContent, RequestFileOrderSourceContent}
import javax.inject.{Inject, Singleton}
import scala.concurrent.Future
import scala.concurrent.duration._
import spray.client.pipelining._
import spray.http.Uri
import spray.httpx.SprayJsonSupport._
import spray.httpx.encoding.{Deflate, Gzip}

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class FileOrderSourceClient @Inject private(implicit actorSystem: ActorSystem) {

  import actorSystem.dispatcher

  private implicit val timeout = 60.seconds
  private val readFilesPipeline = sendReceive ~> decode(Deflate) ~> decode(Gzip) ~> unmarshal[FileOrderSourceContent]

  def readNewFiles(agentUri: String, request: RequestFileOrderSourceContent): Future[FileOrderSourceContent] =
    readFilesPipeline(Post(Uri(s"$agentUri/jobscheduler/agent/fileOrderSource/newFiles"), request))
}
