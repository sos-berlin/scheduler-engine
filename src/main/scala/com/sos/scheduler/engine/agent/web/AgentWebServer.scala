package com.sos.scheduler.engine.agent.web

import akka.actor.{ActorSystem, Props}
import akka.io.{IO, Tcp}
import akka.pattern.ask
import akka.util.Timeout
import com.sos.scheduler.engine.agent.command.{CommandExecutor, CommandXmlExecutor}
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
final class AgentWebServer @Inject private(
  conf: AgentConfiguration,
  commandExecutor: CommandExecutor,
  private implicit val actorSystem: ActorSystem)
extends AutoCloseable {

  private val xmlCommandExecutor = new CommandXmlExecutor(commandExecutor.executeCommand)

  private val webServiceActorRef = actorSystem actorOf Props {
    new AgentWebService.AsActor {
      def executeCommand(command: String) = xmlCommandExecutor.execute(command)
    }
  }

  /**
   * @return Future, completed when Agent has been started and is running.
   */
  def start(): Future[Unit] = {
    implicit val timeout: Timeout = 10.seconds
    val response = IO(Http) ? Http.Bind(webServiceActorRef,
      interface = conf.httpInterfaceRestriction getOrElse "",
      port = conf.httpPort)
    response map {
      case _: Http.Bound ⇒
      case Tcp.CommandFailed(_: Http.Bind) ⇒
        throw new RuntimeException(s"Binding to TCP port ${conf.httpPort } failed. " +
          "Port is possibly in use and not available. " +
          "Switch on DEBUG-level logging for `akka.io.TcpListener` to log the cause")
        // (Akka 2.3.7) TODO: replace by actual exception when Akka #3861 is fixed. See https://www.assembla.com/spaces/akka/tickets/3861
    }
  }

  def close() = {
    //TODO Close HTTP port: IO(Http) ! Unbind in einem Aktor? https://gist.github.com/EECOLOR/8127533
  }
}
