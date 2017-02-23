package com.sos.scheduler.engine.plugins.newwebservice.routes.cpp

import akka.actor._
import com.google.common.base.Charsets._
import com.sos.jobscheduler.base.utils.ScalaUtils.RichThrowable
import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.scheduler.engine.common.async.CallQueue
import com.sos.scheduler.engine.cplusplus.runtime.{CppReference, DisposableCppProxyRegister}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import com.sos.scheduler.engine.kernel.cppproxy.{HttpChunkReaderC, HttpResponseC, SpoolerC}
import com.sos.scheduler.engine.kernel.http.SchedulerHttpRequest
import com.sos.scheduler.engine.kernel.security.SchedulerSecurityLevel
import com.sos.scheduler.engine.plugins.newwebservice.configuration.NewWebServicePluginConfiguration.SchedulerHttpCharset
import com.sos.scheduler.engine.plugins.newwebservice.routes.cpp.CppHttpRoute._
import java.net.URLDecoder
import scala.concurrent.{ExecutionContext, Future, Promise}
import scala.util.control.NonFatal
import scala.util.{Failure, Success}
import spray.http.HttpHeaders.{`Content-Type`, _}
import spray.http.MediaTypes.{`application/javascript`, `image/x-icon`, `text/css`, `text/html`, `text/plain`, `text/xml`}
import spray.http.StatusCodes.InternalServerError
import spray.http.{ContentType, HttpRequest, StatusCode}
import spray.httpx.marshalling.Marshaller
import spray.routing.Directives._
import spray.routing._

trait CppHttpRoute {

  protected def spoolerC: SpoolerC
  protected implicit def schedulerThreadCallQueue: SchedulerThreadCallQueue
  protected implicit def executionContext: ExecutionContext
  protected implicit def actorRefFactory: ActorRefFactory
  protected implicit def disposableCppProxyRegister: DisposableCppProxyRegister

  final def cppHttpRoute: Route =
    requestInstance { request ⇒
      val notifier = new ChunkingActor.Notifier
      onComplete(startCppRequest(request, notifier)) {
        case Success((status, Some((contentType, marshallingBundle)))) ⇒
          respondWithStatus(status) {
            implicit val marshaller = chunkReaderCMarshaller(notifier, contentType)
            complete(marshallingBundle)
          }
        case Success((status, None)) ⇒
          complete(status)
        case Failure(t) ⇒
          logger.warn(t.toString, t)
          complete(InternalServerError → t.toSimplifiedString)
      }
    }

  private def startCppRequest(request: HttpRequest, notifier: ChunkingActor.Notifier): Future[(StatusCode, Option[(ContentType, MarshallingBundle)])] = {
    val schedulerHttpRequest = toSchedulerHttpRequest(request)
    schedulerThreadFuture {
      val httpResponseRef = disposableCppProxyRegister.reference(
        spoolerC.java_execute_http_with_security_level(
          schedulerHttpRequest,
          notifier.schedulerHttpResponse,
          SchedulerSecurityLevel.all.cppName))
      val statusCode = StatusCode.int2StatusCode(httpResponseRef.get.status)
      val response = httpResponseRef.get.chunk_reader match {
        case null ⇒
          dispose(httpResponseRef)
          None
        case chunkReaderC ⇒
          val headers = splitHeaders(httpResponseRef.get.header_string)
          val contentType = cppToContentType(headers)
          val terminated = Promise[Unit]()
          terminated.future onComplete { _ ⇒
            dispose(httpResponseRef)
          }
          Some((contentType, MarshallingBundle(chunkReaderC, terminated)))
        }
      (statusCode, response)
    }
  }

  private def dispose(ref: CppReference[HttpResponseC]): Future[Unit] =
    try
      schedulerThreadFuture {
        try {
          ref.get.close()
          disposableCppProxyRegister.dispose(ref)
        } catch {
          case NonFatal(t) ⇒
            logger.warn(s"httpResponseC.close: $t")
            throw t
        }
      }
    catch {
      case t: CallQueue.ClosedException ⇒
        logger.debug(t.toString)
        Future.successful(())
    }
}

object CppHttpRoute {

  private val logger = Logger(getClass)

  private def toSchedulerHttpRequest(request: HttpRequest): SchedulerHttpRequest = {
    val parameters = request.uri.query.toMap
    val fullPathAndQ = toFullPathAndQuery(request)
    val prefixes = Set("/jobscheduler/master/cpp", "/jobscheduler/engine-cpp")
    val pathAndQuery = prefixes collectFirst {
      case prefix if fullPathAndQ startsWith prefix ⇒ fullPathAndQ.substring(prefix.length)
    } getOrElse {
      throw new RuntimeException(s"Missing one of prefixes $prefixes in path: $fullPathAndQ")
    }
    val headers = (request.headers map { o ⇒ o.lowercaseName → o.value }).toMap
    new SchedulerHttpRequest {
      def hasParameter(key: String) = parameters contains key
      def parameter(key: String) = parameters.getOrElse(key, "")
      def header(key: String) = headers.getOrElse(key.toLowerCase, "")
      def protocol() = request.protocol.value
      def urlPath = pathAndQuery indexOf '?' match {
        case -1 ⇒ pathAndQuery
        case i ⇒ pathAndQuery.substring(0, i + 1)
      }
      def charsetName = request.headers collectFirst { case `Content-Type`(o) ⇒ o.charset.value } getOrElse ""
      def httpMethod = request.method.value
      val body = new String(request.entity.data.toByteArray, ByteCharset)
    }
  }

  private def toFullPathAndQuery(request: HttpRequest): String =
    request.header[`Raw-Request-URI`] map { h ⇒
      URLDecoder.decode(h.value, UTF_8.name)
    } getOrElse
      sys.error("Configuration setting spray.can.server.raw-request-uri-header is needed")

  private def splitHeaders(headers: String): Iterable[(String, String)] =
    headers match {
      case "" ⇒ Nil
      case _ ⇒ headers.split("\r\n") map { _.split(": ", 2) } map { h ⇒ h(0) → h(1) }
    }

  private def cppToContentType(headers: Iterable[(String, String)]): ContentType = {
    val mediaType = headers collectFirst { case ("Content-Type", value) ⇒ value.toLowerCase } match {
      case Some("text/html") ⇒ `text/html`
      case Some("text/html; charset=iso-8859-1") ⇒ `text/html`
      case Some("text/xml") ⇒ `text/xml`
      case Some("text/xml; charset=iso-8859-1") ⇒ `text/xml`
      case Some("text/javascript") ⇒ `application/javascript`
      case Some("text/css") ⇒ `text/css`
      case Some("image/icon") ⇒ `image/x-icon`
      case _ ⇒ `text/plain`
    }
    if (mediaType.isText)
      mediaType withCharset SchedulerHttpCharset
    else
      mediaType
  }

  private def chunkReaderCMarshaller(notifier: ChunkingActor.Notifier, contentTypes: ContentType*)
    (implicit schedulerThreadCallQueue: SchedulerThreadCallQueue,
      actorRefFactory: ActorRefFactory,
      executionContext: ExecutionContext): Marshaller[MarshallingBundle]
  =
    Marshaller.of[MarshallingBundle](contentTypes: _*) {
      case (MarshallingBundle(chunkReaderC, terminatedPromise), contentType, ctx) ⇒
        actorRefFactory.actorOf(Props { new ChunkingActor(chunkReaderC, notifier, contentType, ctx, terminatedPromise) })
    }

  private case class MarshallingBundle(
    chunkReaderC: HttpChunkReaderC,
    terminatedPromise: Promise[Unit])

  private sealed trait Result
  private case class StatusOnly(status: StatusCode) extends Result
}
