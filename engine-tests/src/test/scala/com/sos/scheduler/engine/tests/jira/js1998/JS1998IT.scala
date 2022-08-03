package com.sos.scheduler.engine.tests.jira.js1998

import akka.actor.{ActorSystem, Props}
import akka.io.{IO, Tcp}
import akka.pattern.ask
import akka.util.Timeout
import com.sos.scheduler.engine.common.scalautil.Futures.implicits._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.data.jobchain.{JobChainPath, NodeId}
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest.AgentProcessClassPath
import com.sos.scheduler.engine.tests.jira.js1998.JS1998IT._
import java.net.InetSocketAddress
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.concurrent.duration._
import scala.concurrent.{Future, Promise}
import scala.util.Success
import spray.can.Http
import spray.can.server.ServerSettings
import spray.http.StatusCodes.BadGateway
import spray.routing.HttpServiceActor

/**
  * JS-1998 HTTP proxy for Agent returns HTTP status code 502 "Bad Gateway"
  */
@RunWith(classOf[JUnitRunner])
final class JS1998IT extends FreeSpec with AgentWithSchedulerTest {

  private val proxyCalled = Promise[Unit]()

  private implicit val timeout: Timeout = 10.seconds

  "Agent returns 502" in {
    implicit lazy val actorSystem = controller.instance[ActorSystem]
    val httpProxyPort = findRandomFreeTcpPort()
    bind(new InetSocketAddress("127.0.0.1", httpProxyPort))
      .await(99.s)
    assert(!proxyCalled.isCompleted)

    scheduler.executeXml(
      <process_class name={AgentProcessClassPath.withoutStartingSlash}>
        <remote_schedulers>
          <remote_scheduler remote_scheduler={s"http://127.0.0.1:$httpProxyPort"}/>
          <remote_scheduler remote_scheduler={agentUri}/>
        </remote_schedulers>
      </process_class>)

    val orderRun = startOrder(TestJobChainPath orderKey "1").result await 99.s
    assert(proxyCalled.isCompleted)
    assert(orderRun.nodeId == NodeId("OK"))
  }

  private def bind(address: InetSocketAddress)(implicit actorSystem: ActorSystem): Future[Unit] = {
    val webServerActor = actorSystem.actorOf(Props(new HttoProxy), "WebServer")
    val settings = ServerSettings(actorSystem)
    (IO(Http) ? Http.Bind(webServerActor, address, backlog = 10, Nil, Some(settings)))
      .map {
        case _: Http.Bound => // good
        case failed: Tcp.CommandFailed =>
          sys.error(s"Binding to TCP port $address failed. " +
            "Port is possibly in use and not available. " +
            s"Switch on DEBUG-level logging for `akka.io.TcpListener` to log the cause. $failed")
      }
  }

  private class HttoProxy extends HttpServiceActor {
    val receive = runRoute(complete {
      proxyCalled.tryComplete(Success(()))
      BadGateway
    })
  }
}

private object JS1998IT {
  private val TestJobChainPath = JobChainPath("/test")
}
