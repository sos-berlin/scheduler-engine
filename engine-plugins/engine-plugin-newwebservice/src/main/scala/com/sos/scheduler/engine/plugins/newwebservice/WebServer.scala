package com.sos.scheduler.engine.plugins.newwebservice

import akka.actor.{ActorRef, ActorSystem}
import akka.io.{IO, Tcp}
import akka.pattern.ask
import akka.util.Timeout
import com.google.inject.Injector
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.plugins.newwebservice.WebServer._
import java.net.InetSocketAddress
import scala.concurrent.duration._
import scala.concurrent.{ExecutionContext, Future}
import spray.can.Http
import spray.can.Http.Unbind
import spray.can.server.ServerSettings
import spray.io.ServerSSLEngineProvider

/**
 * @author Joacim Zschimmer
 */
final class WebServer(
  httpAddress: InetSocketAddress,
  injector: Injector)(
  implicit actorSystem: ActorSystem, ec: ExecutionContext)
extends AutoCloseable {

  /**
   * @return Future, completed when Agent has been started and is running.
   */
  def start(): Future[Unit] = {
    bindHttp(httpAddress)
  }

  private def bindHttp(address: InetSocketAddress) =
    bind(address, useHttps = false,
      newWebServiceActorRef(s"EngineWebService-http-${inetSocketAddressToName(address)}"),
      implicitly[ServerSSLEngineProvider])

  private def newWebServiceActorRef(name: String) =
    actorSystem.actorOf(WebServiceActor.props(injector), name)

  private def bind(address: InetSocketAddress, useHttps: Boolean, actorRef: ActorRef, sslEngineProvider: ServerSSLEngineProvider): Future[Unit] = {
    implicit val timeout: Timeout = 10.seconds
    val settings = ServerSettings(actorSystem).copy(sslEncryption = useHttps)
    (IO(Http) ? Http.Bind(actorRef, interface = address.getAddress.getHostAddress, port = address.getPort, settings = Some(settings))(sslEngineProvider))
      .map {
        case _: Http.Bound ⇒  // good
        case failed: Tcp.CommandFailed ⇒
          sys.error(s"Binding to TCP port $address failed: $failed. " +
            "Port is possibly in use and not available. Switch on DEBUG-level logging for `akka.io.TcpListener` to log the cause")
            // (Akka 2.3.7) When Akka #13861 should be fixed, replace by actual exception. See https://github.com/akka/akka/issues/13861
    }
  }

  def close() = {
    implicit val timeout = Timeout(ShutdownTimeout.toFiniteDuration)
    val future = for (_ ← IO(Http) ? Unbind(ShutdownTimeout.toConcurrent);
                      _ ← IO(Http) ? Http.CloseAll) yield ()
    // Does not terminate in time !!!  awaitResult(future, ShutdownTimeout)
  }
}

object WebServer {
  private val ShutdownTimeout = 5.s
  private val logger = Logger(getClass)

  private def inetSocketAddressToName(o: InetSocketAddress): String = o.getAddress.getHostAddress + s":${o.getPort}"
}
