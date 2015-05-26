package com.sos.scheduler.engine.agent.client

import akka.actor.ActorSystem
import com.sos.scheduler.engine.agent.client.AgentClientFactory._
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
final class AgentClientFactory @Inject private(implicit actorSystem: ActorSystem) {

  import actorSystem.dispatcher

  private val pipelineTrunk = sendReceive ~> decode(Deflate) ~> decode(Gzip)
  private val requestFileOrderSourceContentPipeline = pipelineTrunk ~> unmarshal[FileOrderSourceContent]

  def apply(agentUri: String): AgentClient = new AgentClient {
    def executeCommand(command: Command): Future[command.Response] = {
      val response = command match {
        case cmd: RequestFileOrderSourceContent ⇒ exec(cmd)
      }
      response map {_.asInstanceOf[command.Response]}
    }

    private def exec(command: RequestFileOrderSourceContent): Future[FileOrderSourceContent] = {
      val uri = Uri(s"${agentUri stripSuffix "/"}$CommandPath")
      requestFileOrderSourceContentPipeline(Post(uri, command: Command))
    }

    override def toString = s"AgentClient($agentUri)"
  }
}

object AgentClientFactory {
  private val CommandPath = Path("/jobscheduler/agent/command")
}
