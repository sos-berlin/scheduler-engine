package com.sos.scheduler.engine.kernel.agentclient

import akka.actor.ActorSystem
import akka.util.Timeout
import com.sos.scheduler.engine.kernel.agentclient.TestCommandExecutorHttpServer._
import java.net.URI
import javax.inject.Inject
import scala.concurrent.Future
import scala.concurrent.duration._
import spray.routing.SimpleRoutingApp

/**
 * @author Joacim Zschimmer
 */
final class TestCommandExecutorHttpServer private(port: Int, execute: String ⇒ String)(implicit actorSystem: ActorSystem)
extends SimpleRoutingApp {

  import scala.concurrent.ExecutionContext.Implicits.global

  val baseUri = new URI(s"http://$Interface:$port/")

  def start(): Future[Unit] = {
    implicit val timeout: Timeout = 15.seconds
    val serverFuture = startServer(Interface, port) {
      post {
        path("jobscheduler" / "engine" / "command") {
          entity(as[String]) { commandString ⇒
            complete {
              execute(commandString)
            }
          }
        }
      }
    }
    serverFuture map { o ⇒ }
  }
}

private object TestCommandExecutorHttpServer {
  val Interface = "127.0.0.1"

  final class Factory @Inject private(actorSystem: ActorSystem) {
    def apply(port: Int, executor: String ⇒ String) = new TestCommandExecutorHttpServer(port, executor)(actorSystem)
  }
}
