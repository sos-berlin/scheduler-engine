package com.sos.scheduler.engine.agent

import akka.actor.{ActorSystem, Props}
import akka.io.{IO, Tcp}
import akka.pattern.ask
import akka.util.Timeout
import com.sos.scheduler.engine.agent.command.CommandXmlExecutor
import com.sos.scheduler.engine.agent.configuration.AgentConfiguration
import javax.inject.{Inject, Singleton}
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.Future
import scala.concurrent.duration._
import spray.can.Http

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class AgentStarter @Inject private(
  conf: AgentConfiguration,
  commandExecutor: CommandXmlExecutor,
  private implicit val actorSystem: ActorSystem) {

  private val httpServer = new AgentHttpServer
  private implicit val timeout: Timeout = 10.seconds

  def start(): Future[Unit] = {
    httpServer.start()
  }

  private def close() = {
    httpServer.close()
  }

  private class AgentHttpServer extends AutoCloseable {
    private val webServiceActorRef = actorSystem actorOf Props {
      new AgentWebServiceActor(executeCommand = commandExecutor.execute)
    }

    def start(): Future[Unit] =
      IO(Http) ? Http.Bind(webServiceActorRef, interface = conf.httpInterfaceRestriction getOrElse "", port = conf.httpPort) map {
        case _: Http.Bound ⇒
        case Tcp.CommandFailed(_: Http.Bind) ⇒
          // (Akka 2.3.7) TODO: replace by actual exception when Akka #3861 is fixed. See https://www.assembla.com/spaces/akka/tickets/3861
          throw new RuntimeException(s"Binding to TCP port ${conf.httpPort} failed. Port is possibly in use and not available. Switch on DEBUG-level logging for `akka.io.TcpListener` to log the cause")
      }

    def close() = {
      //TODO Close HTTP port: IO(Http) ! Unbind in einem Aktor? https://gist.github.com/EECOLOR/8127533
    }
  }
}
