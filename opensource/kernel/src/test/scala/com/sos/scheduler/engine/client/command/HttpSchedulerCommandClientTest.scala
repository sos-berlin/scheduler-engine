package com.sos.scheduler.engine.client.command

import akka.actor.ActorSystem
import com.google.inject.{AbstractModule, Guice}
import com.sos.scheduler.engine.client.command.HttpSchedulerCommandClientTest._
import com.sos.scheduler.engine.common.inject.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.xmls.SafeXML
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXmls.implicits._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.schedulerEncoding
import com.typesafe.config.ConfigFactory
import org.scalatest.Matchers._
import org.scalatest.{BeforeAndAfterAll, FreeSpec}
import scala.concurrent.duration._
import scala.concurrent.{Await, ExecutionContext, Future}
import scala.util.Failure

/**
 * @author Joacim Zschimmer
 */
final class HttpSchedulerCommandClientTest extends FreeSpec with BeforeAndAfterAll {

  private lazy val httpPort = findRandomFreeTcpPort()
  private lazy val injector = Guice.createInjector(new AbstractModule {
    override def configure(): Unit = {
      bind(classOf[ActorSystem]) toInstance ActorSystem("HttpSchedulerCommandClientTest", AkkaConfig)
      bind(classOf[ExecutionContext]) toInstance ExecutionContext.global
    }
  })
  private lazy val server = injector.apply[TestCommandExecutorHttpServer.Factory].apply(httpPort, executeCommand)
  private lazy val client = injector.apply[HttpSchedulerCommandClient]
  private lazy val uri = s"http://$HttpInterface:$httpPort/"

  override def beforeAll(): Unit = {
    val future = server.start()
    Await.result(future, TestTimeout)
  }

  override def afterAll(): Unit = {
    injector.apply[ActorSystem].shutdown()
  }

  "execute" in {
    expectError(client.execute(uri, TestCommandElem))
  }

  "executeXml" in {
    expectError(client.executeXml(uri, TestCommandElem.toBytes(schedulerEncoding)))
  }

  private def expectError[A](future: Future[A]): Unit = {
    Await.ready(future, Duration.Inf).value.get match {
      case Failure(e: RemoteSchedulers.XmlResponseException) ⇒ e.getMessage shouldEqual ErrorMessage
    }
  }

  "uncheckedExecute" in {
    val startFuture = client.uncheckedExecute(uri, TestCommandElem)
    Await.result(startFuture, Duration.Inf) shouldEqual TestResponseString
  }

  "uncheckedExecuteXml" in {
    val startFuture = client.uncheckedExecuteXml(uri, TestCommandElem.toBytes(schedulerEncoding))
    Await.result(startFuture, Duration.Inf) shouldEqual TestResponseString
  }

  "Server error" in {
    val startFuture = client.uncheckedExecute(uri, <INVALID/>)
    Await.ready(startFuture, TestTimeout).value.get match {
      case Failure(t) ⇒ // Okay
      case o ⇒ fail(o.toString)
    }
  }

  "Client error" in {
    val errorPort = findRandomFreeTcpPort()
    val startFuture = client.uncheckedExecute(s"http://$HttpInterface:$errorPort/", <INVALID/>)
    Await.ready(startFuture, TestTimeout).value.get match {
      case Failure(t) ⇒ // Okay
      case o ⇒ fail(o.toString)
    }
  }
}

private object HttpSchedulerCommandClientTest {
  private val AkkaConfig = ConfigFactory.parseString("spray.can.host-connector.max-retries = 0")
  private val HttpInterface = "127.0.0.1"
  private val TestCommandElem = <COMMAND/>
  private val ErrorMessage = "TEST-ERRÖR"
  private val TestResponseString = <spooler><answer><ERROR text={ErrorMessage}/></answer></spooler>.toString()
  private val TestTimeout = 15.seconds

  private def executeCommand(command: String) = {
    SafeXML.loadString(command) match {
      case TestCommandElem ⇒ TestResponseString
    }
  }
}
