package com.sos.scheduler.engine.client.command

import akka.actor.ActorSystem
import com.google.inject.{AbstractModule, Guice}
import com.sos.scheduler.engine.client.command.HttpSchedulerCommandClientTest._
import com.sos.scheduler.engine.common.inject.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.xmls.SafeXML
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXmls.implicits._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.schedulerEncoding
import java.net.URI
import org.scalatest.Matchers._
import org.scalatest.{BeforeAndAfterAll, FreeSpec}
import scala.concurrent.duration._
import scala.concurrent.{Await, ExecutionContext}
import scala.util.Failure

/**
 * @author Joacim Zschimmer
 */
final class HttpSchedulerCommandClientTest extends FreeSpec with BeforeAndAfterAll {
  private lazy val httpPort = findRandomFreeTcpPort()
  private lazy val injector = Guice.createInjector(new AbstractModule {
    override def configure() {
      bind(classOf[ActorSystem]) toInstance ActorSystem()
      bind(classOf[ExecutionContext]) toInstance ExecutionContext.global
    }
  })
  private lazy val server = injector.apply[TestCommandExecutorHttpServer.Factory].apply(httpPort, executeCommand)
  private lazy val client = injector.apply[HttpSchedulerCommandClient.Factory].apply(new URI(s"http://$HttpInterface:$httpPort/"))

  override def beforeAll() {
    val future = server.start()
    Await.result(future, TestTimeout)
  }

  override def afterAll() {
    injector.apply[ActorSystem].shutdown()
  }

  "execute" in {
    val startFuture = client.execute(TestCommandElem)
    Await.result(startFuture, Duration.Inf) shouldEqual TestResponseString
  }

  "executeXml" in {
    val startFuture = client.executeXml(TestCommandElem.toBytes(schedulerEncoding))
    Await.result(startFuture, Duration.Inf) shouldEqual TestResponseString
  }

  "Server error" in {
    val startFuture = client.execute(<INVALID/>)
    Await.ready(startFuture, TestTimeout).value.get match {
      case Failure(t) ⇒ // Okay
      case o ⇒ fail(o.toString)
    }
  }

  "Client error" in {
    val errorPort = findRandomFreeTcpPort()
    val errorClient = injector.apply[HttpSchedulerCommandClient.Factory].apply(new URI(s"http://$HttpInterface:$errorPort/"))
    val startFuture = errorClient.execute(<INVALID/>)
    Await.ready(startFuture, TestTimeout).value.get match {
      case Failure(t) ⇒ // Okay
      case o ⇒ fail(o.toString)
    }
  }

}

private object HttpSchedulerCommandClientTest {
  private val HttpInterface = "127.0.0.1"
  private val TestCommandElem = <COMMAND/>
  private val TestCommandString = TestCommandElem.toString()
  private val TestResponseString = "<RESPÖNSE/>"
  private val TestTimeout = 15.seconds

  private def executeCommand(command: String) = {
    SafeXML.loadString(command) match {
      case TestCommandElem ⇒ TestResponseString
    }
  }
}
