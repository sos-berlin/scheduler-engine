package com.sos.scheduler.engine.client.command

import akka.actor.ActorSystem
import akka.util.Timeout
import com.sos.jobscheduler.common.sprayutils.XmlString
import com.sos.scheduler.engine.client.command.TestCommandExecutorHttpServer._
import java.net.URI
import javax.inject.{Inject, Singleton}
import scala.concurrent.Future
import scala.concurrent.duration._
import scala.util.control.NonFatal
import spray.http.StatusCodes.BadRequest
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
          decompressRequest() {
            entity(as[XmlString]) { case XmlString(commandString) ⇒
              try {
                val result = execute(commandString)
                complete { XmlString(result) }
              }
              catch {
                case NonFatal(t) ⇒ complete((BadRequest, t.toString))
              }
            }
          }
        }
      }
    }
    serverFuture map { o ⇒ }
  }
}

object TestCommandExecutorHttpServer {
  private val Interface = "127.0.0.1"

  @Singleton
  final class Factory @Inject private(actorSystem: ActorSystem) {
    def apply(port: Int, executor: String ⇒ String) = new TestCommandExecutorHttpServer(port, executor)(actorSystem)
  }
}
