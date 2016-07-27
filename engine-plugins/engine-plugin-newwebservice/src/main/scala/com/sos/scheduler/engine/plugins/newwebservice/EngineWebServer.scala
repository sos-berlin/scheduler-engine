package com.sos.scheduler.engine.plugins.newwebservice

import akka.actor.{ActorRef, ActorSystem}
import com.google.inject.Injector
import com.sos.scheduler.engine.common.sprayutils.WebServerBinding
import com.sos.scheduler.engine.common.sprayutils.web.SprayWebServer
import java.net.InetSocketAddress
import scala.concurrent.ExecutionContext

/**
 * @author Joacim Zschimmer
 */
final class EngineWebServer(
  httpAddress: InetSocketAddress,
  injector: Injector)(
  implicit
    protected val actorSystem: ActorSystem,
    protected val executionContext: ExecutionContext)
extends SprayWebServer {

  protected val bindings = List(WebServerBinding.Http(httpAddress))

  protected def newRouteActorRef(binding: WebServerBinding): ActorRef =
    binding match {
      case WebServerBinding.Http(address) ⇒
        actorSystem.actorOf(
          WebServiceActor.props(injector),
          name = SprayWebServer.actorName("EngineWebServer", binding))
      case _ ⇒
        throw new UnsupportedOperationException
    }
}
