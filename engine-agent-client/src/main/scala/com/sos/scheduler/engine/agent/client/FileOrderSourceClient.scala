package com.sos.scheduler.engine.agent.client

import akka.actor.ActorSystem
import com.sos.scheduler.engine.agent.data.FileOrderSourceContent
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

  def readFiles(agentUri: String, directory: String): Future[FileOrderSourceContent] =
    readFilesPipeline(Get(Uri(s"$agentUri/jobscheduler/agent/fileOrderSource/files").withQuery(
      "directory" → directory,
      "order" → "latest-access-first")))
}
