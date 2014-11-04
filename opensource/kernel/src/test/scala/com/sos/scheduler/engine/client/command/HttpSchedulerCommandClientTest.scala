package com.sos.scheduler.engine.client.command

import akka.actor.ActorSystem
import com.google.inject.{AbstractModule, Guice}
import com.sos.scheduler.engine.client.command.HttpSchedulerCommandClientTest._
import com.sos.scheduler.engine.common.inject.GuiceImplicits._
import com.sos.scheduler.engine.common.scalautil.xmls.SafeXML
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXmls.implicits._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.schedulerEncoding
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
    override def configure(): Unit = {
      bind(classOf[ActorSystem]) toInstance ActorSystem()
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
    val startFuture = client.execute(uri, TestCommandElem)
    Await.result(startFuture, Duration.Inf) shouldEqual TestResponseString
  }

  "executeXml" in {
    val startFuture = client.executeXml(uri, TestCommandElem.toBytes(schedulerEncoding))
    Await.result(startFuture, Duration.Inf) shouldEqual TestResponseString
  }

  "Server error" in {
    val startFuture = client.execute(uri, <INVALID/>)
    Await.ready(startFuture, TestTimeout).value.get match {
      case Failure(t) ⇒ // Okay
      case o ⇒ fail(o.toString)
    }
  }

  "Client error" in {
    val errorPort = findRandomFreeTcpPort()
    val startFuture = client.execute(s"http://$HttpInterface:$errorPort/", <INVALID/>)
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
