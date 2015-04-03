package com.sos.scheduler.engine.client.agent

import akka.actor.ActorSystem
import com.google.inject.{AbstractModule, Guice}
import com.sos.scheduler.engine.client.agent.HttpRemoteProcessStarterTest._
import com.sos.scheduler.engine.client.command.TestCommandExecutorHttpServer
import com.sos.scheduler.engine.common.guice.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.Futures._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import org.junit.runner.RunWith
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import org.scalatest.{BeforeAndAfterAll, FreeSpec}
import scala.concurrent.ExecutionContext
import scala.concurrent.duration._

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class HttpRemoteProcessStarterTest extends FreeSpec with BeforeAndAfterAll {
  private lazy val httpPort = findRandomFreeTcpPort()
  private lazy val injector = Guice.createInjector(new AbstractModule {
    override def configure(): Unit = {
      bind(classOf[ActorSystem]) toInstance ActorSystem()
      bind(classOf[ExecutionContext]) toInstance ExecutionContext.global
    }
  })
  private lazy val commandExecutor = new CommandExecutor
  private lazy val server = injector.instance[TestCommandExecutorHttpServer.Factory].apply(httpPort, commandExecutor)
  private lazy val conf = ApiProcessConfiguration(
    hasApi = false,
    javaOptions = DummyJavaOptions,
    javaClasspath = DummyJavaClasspath)
  private lazy val client = injector.instance[HttpRemoteProcessStarter]

  override def beforeAll(): Unit = {
    val future = server.start()
    awaitResult(future, 15.seconds)
  }

  override def afterAll(): Unit = {
    injector.instance[ActorSystem].shutdown()
  }

  "startRemoteTask and closeRemoteTask" in {
    val startFuture = client.startRemoteTask(schedulerApiTcpPort = DummyApiTcpPort, conf, remoteUri = server.baseUri.toString)
    val httpRemoteProcess: HttpRemoteProcess = awaitResult(startFuture, Duration.Inf)
    val closeFuture = httpRemoteProcess.closeRemoteTask(kill = true)
    awaitResult(closeFuture, Duration.Inf)
    commandExecutor.startCommandReceived shouldBe true
    commandExecutor.closeCommandReceived shouldBe true
  }
}

private object HttpRemoteProcessStarterTest {
  private val DummyApiTcpPort = 9999
  private val DummyJavaOptions = "JAVA-OPTIONS"
  private val DummyJavaClasspath = "JAVA-CLASSPATH"
  private val ExpectedStartCommand = s"""<remote_scheduler.start_remote_task tcp_port="$DummyApiTcpPort" kind="process" java_options="$DummyJavaOptions" java_classpath="$DummyJavaClasspath"/>"""

  private class CommandExecutor extends (String ⇒ String) {
    @volatile var startCommandReceived = false
    @volatile var closeCommandReceived = false

    def apply(commandString: String) =
      commandString match {
        case ExpectedStartCommand ⇒
          startCommandReceived = true
          <spooler><answer><process process_id={"1111"} pid={"2222"}/></answer></spooler>.toString()
        case """<remote_scheduler.remote_task.close process_id="1111" kill="true"/>""" ⇒
          closeCommandReceived = true
          <spooler><answer><ok/></answer></spooler>.toString()
      }
  }
}
