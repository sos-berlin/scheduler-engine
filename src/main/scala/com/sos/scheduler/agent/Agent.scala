package com.sos.scheduler.agent

import akka.actor.{ActorSystem, Props}
import akka.io.{IO, Tcp}
import akka.pattern.ask
import akka.util.Timeout
import com.sos.scheduler.agent.command.StringCommandExecutor
import com.sos.scheduler.agent.configuration.AgentConfiguration
import javax.inject.{Inject, Singleton}
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.Future
import scala.concurrent.duration._
import spray.can.Http

/**
 * @author Joacim Zschimmer
 */
@Singleton
final class Agent @Inject private(
  conf: AgentConfiguration,
  executeStringCommand: StringCommandExecutor,
  private implicit val actorSystem: ActorSystem) {

  private val webServiceActorRef = actorSystem actorOf Props(new AgentWebServiceActor(executeCommand = executeStringCommand))
  private implicit val timeout: Timeout = 10.seconds

  def start(): Future[Unit] = {
    IO(Http) ask Http.Bind(webServiceActorRef, interface = conf.httpInterfaceRestriction getOrElse "", port = conf.httpPort) flatMap {
      case b: Http.Bound ⇒ Future.successful(())
      case Tcp.CommandFailed(b: Http.Bind) ⇒
        // TODO: replace by actual exception when Akka #3861 is fixed.
        //       see https://www.assembla.com/spaces/akka/tickets/3861
        Future.failed(new RuntimeException("Binding failed. Switch on DEBUG-level logging for `akka.io.TcpListener` to log the cause."))
    }
  }
}
