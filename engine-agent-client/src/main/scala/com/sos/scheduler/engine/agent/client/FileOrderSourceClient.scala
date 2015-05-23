package com.sos.scheduler.engine.agent.client

import akka.actor.ActorSystem
import com.sos.scheduler.engine.agent.client.FileOrderSourceClient._
import com.sos.scheduler.engine.agent.data.commands._
import javax.inject.{Inject, Singleton}
import scala.concurrent.Future
import spray.client.pipelining._
import spray.http.Uri
import spray.http.Uri.Path
import spray.httpx.SprayJsonSupport._
import spray.httpx.encoding.{Deflate, Gzip}

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class FileOrderSourceClient @Inject private(implicit actorSystem: ActorSystem) {

  import actorSystem.dispatcher

  private val pipelineTrunk = sendReceive ~> decode(Deflate) ~> decode(Gzip)
  private val requestFileOrderSourceContentPipeline = pipelineTrunk ~> unmarshal[FileOrderSourceContent]

  def execute(agentUri: String, command: Command): Future[command.Response] = {
    val response = command match {
      case cmd: RequestFileOrderSourceContent ⇒ exec(agentUri, cmd)
    }
    response map { _.asInstanceOf[command.Response] }
  }

  def exec(agentUri: String, command: RequestFileOrderSourceContent): Future[FileOrderSourceContent] = {
    val uri = Uri(s"${agentUri stripSuffix "/"}$CommandPath")
    requestFileOrderSourceContentPipeline(Post(uri, command: Command))
  }
}

object FileOrderSourceClient {
  private val CommandPath = Path("/jobscheduler/agent/command")
}
