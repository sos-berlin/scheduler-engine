package com.sos.scheduler.engine.agent.client

import akka.actor.ActorRefFactory
import akka.util.Timeout
import com.sos.scheduler.engine.agent.client.AgentClient._
import com.sos.scheduler.engine.agent.data.AgentTaskId
import com.sos.scheduler.engine.agent.data.commandresponses.{EmptyResponse, FileOrderSourceContent, StartTaskResponse}
import com.sos.scheduler.engine.agent.data.commands._
import com.sos.scheduler.engine.agent.data.views.{TaskHandlerOverview, TaskOverview}
import com.sos.scheduler.engine.agent.data.web.AgentUris
import com.sos.scheduler.engine.base.generic.SecretString
import com.sos.scheduler.engine.base.utils.ScalazStyle.OptionRichBoolean
import com.sos.scheduler.engine.common.auth.{UserAndPassword, UserId}
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.soslicense.LicenseKeyString
import com.sos.scheduler.engine.common.sprayutils.SimpleTypeSprayJsonSupport._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.IntelliJUtils.intelliJuseImports
import java.time.Duration
import org.scalactic.Requirements._
import scala.collection.immutable
import scala.concurrent.Future
import spray.client.pipelining._
import spray.http.CacheDirectives.{`no-cache`, `no-store`}
import spray.http.HttpHeaders.{Accept, `Cache-Control`}
import spray.http.MediaTypes._
import spray.http.StatusCodes.InternalServerError
import spray.http._
import spray.httpx.SprayJsonSupport._
import spray.httpx.UnsuccessfulResponseException
import spray.httpx.encoding.Gzip
import spray.httpx.unmarshalling._
import spray.json.DefaultJsonProtocol._
import spray.json.JsBoolean

/**
 * Client for JobScheduler Agent.
 * The HTTP requests are considerd to be responded within `RequestTimeout`.
 * The command [[RequestFileOrderSourceContent]] has an own timeout, which is used for the HTTP request, too (instead of `RequestTimeout`).
 *
 * @author Joacim Zschimmer
 */
trait AgentClient {
  import actorRefFactory.dispatcher

  val agentUri: Uri
  protected def licenseKeys: immutable.Iterable[LicenseKeyString]
  implicit protected val actorRefFactory: ActorRefFactory
  protected def userAndPasswordOption: Option[UserAndPassword] = None

  protected lazy val agentUris = AgentUris(agentUri.toString)
  private lazy val addLicenseKeys: RequestTransformer = if (licenseKeys.nonEmpty) addHeader(AgentUris.LicenseKeyHeaderName, licenseKeys mkString " ")
    else identity
  lazy val addUserAndPassword: RequestTransformer = userAndPasswordOption match {
    case Some(UserAndPassword(UserId(user), SecretString(password))) ⇒ addCredentials(BasicHttpCredentials(user, password))
    case None ⇒ identity
  }
  private lazy val httpResponsePipeline: HttpRequest ⇒ Future[HttpResponse] =
    addUserAndPassword ~>
      addLicenseKeys ~>
      encode(Gzip) ~>
      sendReceive ~>
      decode(Gzip)
  private lazy val nonCachingHttpResponsePipeline: HttpRequest ⇒ Future[HttpResponse] =
    addHeader(Accept(`application/json`)) ~>
      addHeader(`Cache-Control`(`no-cache`, `no-store`)) ~>   // Unnecessary ?
      httpResponsePipeline

  final def executeCommand(command: Command): Future[command.Response] = {
    logger.debug(s"Execute $command")
    val response = command match {
      case command: RequestFileOrderSourceContent ⇒ executeRequestFileOrderSourceContent(command)
      case command: StartTask ⇒ unmarshallingPipeline[StartTaskResponse].apply(Post(agentUris.command, command: Command))
      case (_: DeleteFile | _: MoveFile | _: SendProcessSignal | _: CloseTask | _: Terminate | AbortImmediately) ⇒
        unmarshallingPipeline[EmptyResponse.type].apply(Post(agentUris.command, command: Command))
    }
    response map { _.asInstanceOf[command.Response] } recover {
      case e: UnsuccessfulResponseException if e.response.status == InternalServerError ⇒
        import e.response.entity
        val message = if (entity.data.length < 1024) entity.asString else entity.data.length + " bytes"
        throw new RuntimeException(s"HTTP-${e.response.status}: $message")
    }
  }

  private def executeRequestFileOrderSourceContent(command: RequestFileOrderSourceContent): Future[FileOrderSourceContent] = {
    val timeout = commandDurationToRequestTimeout(command.duration)
    val pipeline =
      addUserAndPassword ~>
        addHeader(Accept(`application/json`)) ~>
        addLicenseKeys ~>
        encode(Gzip) ~>
        sendReceive(actorRefFactory, actorRefFactory.dispatcher, timeout) ~>
        decode(Gzip) ~>
        unmarshal[FileOrderSourceContent]
    pipeline(Post(agentUris.command, command: Command))
  }

  final def fileExists(filePath: String): Future[Boolean] =
    unmarshallingPipeline[JsBoolean].apply(Get(agentUris.fileExists(filePath))) map { _.value }

  object task {
    final def overview: Future[TaskHandlerOverview] = get[TaskHandlerOverview](_.task.overview)

    final def tasks: Future[immutable.Seq[TaskOverview]] = get[immutable.Seq[TaskOverview]](_.task.tasks)

    final def apply(id: AgentTaskId): Future[TaskOverview] = get[TaskOverview](_.task(id))
  }

  final def get[A: FromResponseUnmarshaller](uri: AgentUris ⇒ String): Future[A] =
    unmarshallingPipeline[A].apply(Get(uri(agentUris)))

  final def apply[A: FromResponseUnmarshaller](request: HttpRequest): Future[A] =
    withCheckedAgentUri(request) { request ⇒
      unmarshallingPipeline[A].apply(request)
    }

  final def apply[A: FromResponseUnmarshaller](headers: List[HttpHeader], request: HttpRequest): Future[A] =
    withCheckedAgentUri(request) { request ⇒
      (addHeaders(headers) ~> httpResponsePipeline ~> unmarshal[A]).apply(request)
    }

  private def withCheckedAgentUri[A](request: HttpRequest)(body: HttpRequest ⇒ Future[A]): Future[A] =
    toCheckedAgentUri(request.uri) match {
      case Some(uri) ⇒ body(request.copy(uri = uri))
      case None ⇒ Future.failed(new IllegalArgumentException(s"URI '${request.uri} does not match $toString"))
    }


  private[client] def toCheckedAgentUri(uri: Uri): Option[Uri] = {
    var u = normalizeAgentUri(uri)
    checkAgentUri(u)
  }

  private[client] def normalizeAgentUri(uri: Uri): Uri = {
    val Uri(scheme, authority, path, query, fragment) = uri
    if (scheme.isEmpty && authority.isEmpty)
      Uri(agentUri.scheme, agentUri.authority, path, query, fragment)
    else
      uri
  }

  private[client] def checkAgentUri(uri: Uri): Option[Uri] = {
    val myAgentUri = Uri(scheme = uri.scheme, authority = uri.authority)
    myAgentUri == agentUri && uri.path.toString.startsWith(agentUris.prefixedUri.path.toString) option
      uri
  }

  private def unmarshallingPipeline[A: FromResponseUnmarshaller] = nonCachingHttpResponsePipeline ~> unmarshal[A]

  override def toString = s"AgentClient($agentUri)"
}

object AgentClient {
  intelliJuseImports(StringJsonFormat)  // for import spray.json.DefaultJsonProtocol._

  val RequestTimeout = 60.s
  //private val RequestTimeoutMaximum = Int.MaxValue.ms  // Limit is the number of Akka ticks, where a tick can be as short as 1ms (see akka.actor.LightArrayRevolverScheduler.checkMaxDelay)
  private val logger = Logger(getClass)

  final class Standard(
    val agentUri: Uri,
    protected val licenseKeys: immutable.Iterable[LicenseKeyString] = Nil,
    override val userAndPasswordOption: Option[UserAndPassword] = None)
    (implicit protected val actorRefFactory: ActorRefFactory)
  extends AgentClient

  /**
   * The returns timeout for the HTTP request is longer than the expected duration of the request
   */
  private[client] def commandDurationToRequestTimeout(duration: Duration): Timeout = {
    require(duration >= 0.s)
    Timeout((duration + RequestTimeout).toFiniteDuration)
  }
}
