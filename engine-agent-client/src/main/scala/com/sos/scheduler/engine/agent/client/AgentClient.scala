package com.sos.scheduler.engine.agent.client

import akka.actor.ActorSystem
import akka.util.Timeout
import com.sos.scheduler.engine.agent.client.AgentClient._
import com.sos.scheduler.engine.agent.data.commands._
import com.sos.scheduler.engine.agent.data.responses.{EmptyResponse, FileOrderSourceContent}
import com.sos.scheduler.engine.common.scalautil.Futures.awaitResult
import com.sos.scheduler.engine.common.time.ScalaTime._
import org.scalactic.Requirements._
import scala.concurrent.Future
import spray.client.pipelining._
import spray.http.CacheDirectives.{`no-cache`, `no-store`}
import spray.http.HttpHeaders.{Accept, `Cache-Control`}
import spray.http.MediaTypes._
import spray.http.StatusCodes.{NotFound, OK}
import spray.http.{HttpEntity, HttpRequest, HttpResponse, Uri}
import spray.httpx.SprayJsonSupport._
import spray.httpx.encoding.Gzip
import spray.httpx.marshalling.Marshaller

/**
 * Client for JobScheduler Agent.
 * The HTTP requests are considerd to be responded within `RequestTimeout`.
 * The command [[RequestFileOrderSourceContent]] has an own timeout, which is used for the HTTP request, too (instead of `RequestTimeout`).
 *
 * @author Joacim Zschimmer
 */
trait AgentClient {
  import actorSystem.dispatcher

  protected[client] val agentUri: String
  implicit protected val actorSystem: ActorSystem

  private lazy val baseUri: String = agentUri stripSuffix "/"
  private lazy val commandUri = baseUri concat CommandPath
  private lazy val nonCachingHttpResponsePipeline: HttpRequest ⇒ Future[HttpResponse] =
    addHeader(Accept(`application/json`)) ~>
      addHeader(`Cache-Control`(`no-cache`, `no-store`)) ~>
      encode(Gzip) ~>
      sendReceive ~>
      decode(Gzip)

  /**
   * Sends a JSON string containing a command to the Agent and awaits the response, returning it as a JSON string.
   * The HTTP request is considerd to be responded within `RequestTimeout`.
   */
  def executeJsonCommandSynchronously(commandJson: String): String =
    awaitResult(executeJsonCommand(commandJson), 2 * RequestTimeout)

  private def executeJsonCommand(commandJson: String): Future[String] =
    (nonCachingHttpResponsePipeline ~> unmarshal[String]).apply(Post(commandUri, commandJson)(StringToJsonMarshaller))

  final def executeCommand(command: Command): Future[command.Response] = {
    val response = command match {
      case command: RequestFileOrderSourceContent ⇒ executeRequestFileOrderSourceContent(command)
      case _: Terminate | AbortImmediately ⇒ (nonCachingHttpResponsePipeline ~> unmarshal[EmptyResponse.type]).apply(Post(commandUri, command: Command))
    }
    response map { _.asInstanceOf[command.Response] }
  }

  private def executeRequestFileOrderSourceContent(command: RequestFileOrderSourceContent): Future[FileOrderSourceContent] = {
    val timeout = commandMillisToRequestTimeout(command.durationMillis)
    val pipeline =
      addHeader(Accept(`application/json`)) ~>
        encode(Gzip) ~>
        sendReceive(actorSystem, actorSystem.dispatcher, timeout) ~>
        decode(Gzip) ~>
        unmarshal[FileOrderSourceContent]
    pipeline(Post(commandUri, command: Command))
  }

  final def fileExists(filePath: String): Future[Boolean] =
    nonCachingHttpResponsePipeline(Get(Uri(s"$baseUri$FileStatusPath").withQuery("file" → filePath))) map { httpResponse ⇒
      httpResponse.status match {
        case OK ⇒ true
        case NotFound ⇒ false
        case status ⇒ sys.error(s"HTTP OK or NotFound expected, instead of ${httpResponse.status} ${httpResponse.message}")
      }
    }

  override def toString = s"AgentClient($agentUri)"
}

object AgentClient {
  val RequestTimeout = 60.s
  //private val RequestTimeoutMaximum = Int.MaxValue.ms  // Limit is the number of Akka ticks, where a tick can be as short as 1ms (see akka.actor.LightArrayRevolverScheduler.checkMaxDelay)
  private val CommandPath = "/jobscheduler/agent/command"
  private val FileStatusPath = "/jobscheduler/agent/fileStatus"

  private val StringToJsonMarshaller = Marshaller.of[String](`application/json`) { (string, contentType, ctx) ⇒
    ctx.marshalTo(HttpEntity(contentType, string))
  }

  /**
   * The returns timeout for the HTTP request is longer than the expected duration of the request
   */
  private[client] def commandMillisToRequestTimeout(millis: Long): Timeout = {
    require(millis >= 0)
    val m = (BigDecimal(millis).setScale(3) + RequestTimeout.toMillis) / 1000
    Timeout(bigDecimalToDuration(m).toFiniteDuration)
  }
}
