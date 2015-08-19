package com.sos.scheduler.engine.agent.client

import akka.actor.ActorSystem
import com.sos.scheduler.engine.agent.client.TextAgentClient._
import com.sos.scheduler.engine.common.scalautil.Futures.awaitResult
import com.sos.scheduler.engine.common.sprayutils.YamlJsonConversion.yamlToJsValue
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.JavaResource
import com.typesafe.config.ConfigFactory
import java.nio.charset.StandardCharsets._
import scala.concurrent.Future
import spray.client.pipelining._
import spray.http.CacheDirectives.{`no-cache`, `no-store`}
import spray.http.HttpEntity
import spray.http.HttpHeaders.{Accept, `Cache-Control`}
import spray.http.MediaTypes._
import spray.httpx.encoding.Gzip
import spray.httpx.marshalling.Marshaller
import spray.json.{JsValue, JsonParser}

/**
 * @author Joacim Zschimmer
 */
private[client] class TextAgentClient(agentUri: String, print: String ⇒ Unit) extends AutoCloseable {

  private val agentUris = AgentUris(agentUri)
  private implicit val actorSystem = ActorSystem("AgentClient", ConfigFactory.load(ConfigurationResource.path))
  import actorSystem.dispatcher
  private val pipeline = addHeader(Accept(`text/plain`)) ~> addHeader(`Cache-Control`(`no-cache`, `no-store`)) ~>
      encode(Gzip) ~> sendReceive ~> decode(Gzip) ~> unmarshal[String]
  private var needYamlDocumentSeparator = false

  def close() = actorSystem.shutdown()

  def executeCommand(command: String): Unit =
    doPrint(resultString(pipeline(Post(agentUris.command, forceToJson(command)))))

  def get(uri: String): Unit =
    doPrint(resultString(pipeline(Get(agentUris.api(uri)))))

  def requireIsResponding(): Unit = {
    resultString(pipeline(Get(agentUris.overview)))
    print("JobScheduler Agent is responding")
  }

  private def doPrint(string: String): Unit = {
    if (needYamlDocumentSeparator) print("---")
    needYamlDocumentSeparator = true
    print(string.trim)
  }
}

object TextAgentClient {
  private val ConfigurationResource = JavaResource("com/sos/scheduler/engine/agent/client/main/akka.conf")

  implicit val JsValueMarshaller = Marshaller.of[JsValue](`application/json`) { (value, contentType, ctx) ⇒
    ctx.marshalTo(HttpEntity(`application/json`, value.compactPrint.getBytes(UTF_8)))
  }

  private def forceToJson(jsonOrYaml: String): JsValue =
    try JsonParser(jsonOrYaml)
    catch {
      case _: Exception ⇒ yamlToJsValue(jsonOrYaml)
    }

  private def resultString(future: Future[String]) = awaitResult(future, 2 * AgentClient.RequestTimeout)
}
