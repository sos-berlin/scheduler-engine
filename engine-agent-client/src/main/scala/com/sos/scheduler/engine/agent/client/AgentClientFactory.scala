package com.sos.scheduler.engine.agent.client

import akka.actor.ActorSystem
import akka.util.Timeout
import com.sos.scheduler.engine.agent.client.AgentClientFactory._
import com.sos.scheduler.engine.agent.data.commands._
import javax.inject.{Inject, Singleton}
import scala.concurrent.Future
import com.sos.scheduler.engine.common.time.ScalaTime._
import org.scalactic.Requirements._
import spray.client.pipelining._
import spray.http.Uri
import spray.httpx.SprayJsonSupport._
import spray.httpx.encoding.{Deflate, Gzip}

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class AgentClientFactory @Inject private(implicit actorSystem: ActorSystem) {

  import actorSystem.dispatcher


  def apply(agentUri: String): AgentClient = new AgentClient {
    private val baseUri: String = agentUri stripSuffix "/"

    def executeCommand(command: Command): Future[command.Response] = {
      val response = command match {
        case cmd: RequestFileOrderSourceContent ⇒ executeRequestFileOrderSourceContent(cmd)
      }
      response map { _.asInstanceOf[command.Response] }
    }

    private def executeRequestFileOrderSourceContent(cmd: RequestFileOrderSourceContent): Future[FileOrderSourceContent] = {
      val timeout = commandMillisToRequestTimeout(cmd.durationMillis)
      val pipeline = sendReceive(actorSystem, actorSystem.dispatcher, timeout) ~>
        decode(Deflate) ~> decode(Gzip) ~> unmarshal[FileOrderSourceContent]
      pipeline(Post(baseUri concat CommandPath, cmd: Command))
    }

    override def toString = s"AgentClient($agentUri)"
  }
}

object AgentClientFactory {
  private[client] val RequestTimeout = 60.s
  //private val RequestTimeoutMaximum = Int.MaxValue.ms  // Limit is the number of Akka ticks, where a tick can be as short as 1ms (see akka.actor.LightArrayRevolverScheduler.checkMaxDelay)
  private val CommandPath = "/jobscheduler/agent/command"

  /**
   * The returns timeout for the HTTP request is longer than the expected duration of the request
   */
  private[client] def commandMillisToRequestTimeout(millis: Long): Timeout = {
    require(millis >= 0)
    val m = (BigDecimal(millis) + RequestTimeout.toMillis)
    Timeout(bigDecimalToDuration(m.setScale(3) / 1000))
  }
}
