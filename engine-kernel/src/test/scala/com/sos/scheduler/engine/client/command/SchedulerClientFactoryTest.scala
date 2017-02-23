package com.sos.scheduler.engine.client.command

import akka.actor.{ActorRefFactory, ActorSystem}
import com.google.inject.{AbstractModule, Guice, Provides}
import com.sos.jobscheduler.common.guice.GuiceImplicits._
import com.sos.jobscheduler.common.scalautil.Futures._
import com.sos.jobscheduler.common.scalautil.xmls.SafeXML
import com.sos.jobscheduler.common.scalautil.xmls.ScalaXmls.implicits._
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.jobscheduler.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.client.command.SchedulerClientFactoryTest._
import com.sos.scheduler.engine.client.web.WebCommandClient
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.schedulerEncoding
import com.typesafe.config.ConfigFactory
import javax.inject.Singleton
import org.junit.runner.RunWith
import org.scalatest.Inside.inside
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import org.scalatest.{BeforeAndAfterAll, FreeSpec}
import scala.concurrent.duration._
import scala.concurrent.{Await, ExecutionContext, Future}
import scala.util.Failure

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class SchedulerClientFactoryTest extends FreeSpec with BeforeAndAfterAll {

  private lazy val injector = Guice.createInjector(new AbstractModule {
    def configure() {}

    @Provides @Singleton
    def executionContext(actorSystem: ActorSystem): ExecutionContext = actorSystem.dispatcher

    @Provides @Singleton
    def actorRefFactory(actorSystem: ActorSystem): ActorRefFactory = actorSystem

    @Provides @Singleton
    def actorSystem(): ActorSystem = ActorSystem("SchedulerClientFactoryTest", AkkaConfig)
  })
  private lazy val httpPort = findRandomFreeTcpPort()
  private lazy val server = injector.instance[TestCommandExecutorHttpServer.Factory].apply(httpPort, executeCommand)
  private lazy val clientFactory = injector.instance[SchedulerClientFactory]
  private lazy val uri = s"http://$HttpInterface:$httpPort/"
  private lazy val client = clientFactory.apply(uri)

  override def beforeAll() = {
    val future = server.start()
    awaitResult(future, TestTimeout)
  }

  override def afterAll() = {
    injector.instance[ActorSystem].shutdown()
  }

  "execute" in {
    expectError(client.execute(TestCommandElem))
  }

  "executeXml" in {
    expectError(client.executeXml(TestCommandElem.toByteString(schedulerEncoding)))
  }

  private def expectError[A](future: Future[A]): Unit =
    inside(Await.ready(future, Duration.Inf).value.get) {
      case Failure(e: WebCommandClient.XmlException) ⇒ e.getMessage shouldEqual ErrorMessage
    }

  "uncheckedExecute" in {
    val startFuture = client.uncheckedExecute(TestCommandElem)
    awaitResult(startFuture, MaxDuration) shouldEqual TestResponseString
  }

  "uncheckedExecuteXml" in {
    val startFuture = client.uncheckedExecuteXml(TestCommandElem.toByteString(schedulerEncoding))
    awaitResult(startFuture, MaxDuration) shouldEqual TestResponseString
  }

  "Server error" in {
    val startFuture = client.uncheckedExecute(<INVALID/>)
    Await.ready(startFuture, TestTimeout.toConcurrent).value.get match {
      case Failure(t) ⇒ // Okay
      case o ⇒ fail(o.toString)
    }
  }

  "Client error" in {
    val errorPort = findRandomFreeTcpPort()
    val startFuture = clientFactory.apply(s"http://$HttpInterface:$errorPort/").uncheckedExecute(<INVALID/>)
    Await.ready(startFuture, TestTimeout.toConcurrent).value.get match {
      case Failure(t) ⇒ // Okay
      case o ⇒ fail(o.toString)
    }
  }
}

private object SchedulerClientFactoryTest {
  private val AkkaConfig = ConfigFactory.parseString("spray.can.host-connector.max-retries = 0")
  private val HttpInterface = "127.0.0.1"
  private val TestCommandElem = <COMMAND/>
  private val ErrorMessage = "TEST-ERRÖR"
  private val TestResponseString = <spooler><answer><ERROR text={ErrorMessage}/></answer></spooler>.toString()
  private val TestTimeout = 15.s

  private def executeCommand(command: String) =
    SafeXML.loadString(command) match {
      case TestCommandElem ⇒ TestResponseString
    }
}
