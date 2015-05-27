package com.sos.scheduler.engine.agent.client

import akka.actor.ActorSystem
import akka.util.Timeout
import com.sos.scheduler.engine.agent.client.AgentClientFactory._
import com.sos.scheduler.engine.agent.data.commands._
import com.sos.scheduler.engine.common.time.ScalaTime._
import javax.inject.{Inject, Singleton}
import org.scalactic.Requirements._
import scala.concurrent.Future
import spray.client.pipelining._
import spray.http.CacheDirectives.`no-cache`
import spray.http.HttpHeaders.`Cache-Control`
import spray.http.StatusCodes.{NotFound, OK}
import spray.http.Uri
import spray.httpx.SprayJsonSupport._
import spray.httpx.encoding.{Deflate, Gzip}

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class AgentClientFactory @Inject private(implicit actorSystem: ActorSystem) {

  import actorSystem.dispatcher

  private val httpResponsePipeline = addHeader(`Cache-Control`(`no-cache`)) ~> sendReceive ~> decode(Deflate) ~> decode(Gzip)

  def apply(agentUri: String): AgentClient = new AgentClient {
    private val baseUri: String = agentUri stripSuffix "/"

    def executeCommand(command: Command): Future[command.Response] = {
      val response = command match {
        case cmd: RequestFileOrderSourceContent ⇒ executeRequestFileOrderSourceContent(cmd)
      }
      response map { _.asInstanceOf[command.Response] }
    }

    private def executeRequestFileOrderSourceContent(command: RequestFileOrderSourceContent): Future[FileOrderSourceContent] = {
      val timeout = commandMillisToRequestTimeout(command.durationMillis)
      val pipeline = encode(Gzip) ~> sendReceive(actorSystem, actorSystem.dispatcher, timeout) ~>
        decode(Gzip) ~> unmarshal[FileOrderSourceContent]
      pipeline(Post(baseUri concat CommandPath, command: Command))
    }

    def fileExists(filePath: String): Future[Boolean] =
      httpResponsePipeline(Get(Uri(s"$baseUri$FileStatusPath").withQuery("file" → filePath))) map { r ⇒
        r.status match {
          case OK ⇒ true
          case NotFound ⇒ false
          case status ⇒ sys.error(s"HTTP OK or NotFound expected, instead of ${r.status} ${r.message}")
        }
      }

    override def toString = s"AgentClient($agentUri)"
  }
}

object AgentClientFactory {
  private[client] val RequestTimeout = 60.s
  //private val RequestTimeoutMaximum = Int.MaxValue.ms  // Limit is the number of Akka ticks, where a tick can be as short as 1ms (see akka.actor.LightArrayRevolverScheduler.checkMaxDelay)
  private val CommandPath = "/jobscheduler/agent/command"
  private val FileStatusPath = "/jobscheduler/agent/fileStatus"

  /**
   * The returns timeout for the HTTP request is longer than the expected duration of the request
   */
  private[client] def commandMillisToRequestTimeout(millis: Long): Timeout = {
    require(millis >= 0)
    Timeout(bigDecimalToDuration((BigDecimal(millis).setScale(3) + RequestTimeout.toMillis) / 1000))
  }
}
