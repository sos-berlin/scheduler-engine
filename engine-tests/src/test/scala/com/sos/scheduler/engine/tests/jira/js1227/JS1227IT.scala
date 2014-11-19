package com.sos.scheduler.engine.tests.jira.js1227

import akka.actor.ActorSystem
import com.sos.scheduler.engine.client.command.HttpSchedulerCommandClient
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.HasCloser.implicits._
import com.sos.scheduler.engine.common.scalautil.xmls.SafeXML
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPorts
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.{OrderState, OrderStepEndedEvent, OrderSuspendedEvent, OrderTouchedEvent}
import com.sos.scheduler.engine.data.xmlcommands.{ModifyJobCommand, ModifyOrderCommand, OrderCommand, XmlCommand}
import com.sos.scheduler.engine.kernel.extrascheduler.ExtraScheduler
import com.sos.scheduler.engine.kernel.persistence.hibernate.HibernateOrderStore
import com.sos.scheduler.engine.kernel.persistence.hibernate.ScalaHibernate._
import com.sos.scheduler.engine.main.CppBinary
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.SchedulerTestUtils.awaitSuccess
import com.sos.scheduler.engine.test.TestEnvironment
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.database.H2DatabaseServer
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.tests.jira.js1227.JS1227IT._
import javax.persistence.EntityManagerFactory
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * JS-1227 An order executed on any cluster member should be suspendable.
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1227IT extends FreeSpec with ScalaSchedulerTest {

  import controller.eventBus

  private lazy val Seq(tcpPort, otherTcpPort, otherHttpPort, databasePort) = findRandomFreeTcpPorts(4)
  private lazy val databaseConfiguration = new H2DatabaseServer.Configuration {
    def directory = testEnvironment.databaseDirectory
    def tcpPort = databasePort
  }
  private lazy val databaseServer = new H2DatabaseServer(databaseConfiguration)
  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-tcp-port=$tcpPort", s"-udp-port=$tcpPort", "-distributed-orders"),
    database = Some(databaseConfiguration))
  private lazy val bScheduler = {
    val args = List(
      controller.cppBinaries.file(CppBinary.exeFilename).getPath,
      s"-sos.ini=${testEnvironment.sosIniFile}",
      s"-ini=${testEnvironment.iniFile}",
      s"-id=${TestEnvironment.TestSchedulerId}",
      s"-roles=agent",
      s"-log-dir=${testEnvironment.logDirectory}",
      s"-log-level=debug9",
      s"-log=${testEnvironment.schedulerLog}",
      s"-java-classpath=${System.getProperty("java.class.path")}",
      s"-job-java-classpath=${System.getProperty("java.class.path")}",
      s"-distributed-orders",
      s"-roles=scheduler",
      s"-db=jdbc -class=org.h2.Driver ${databaseServer.jdbcUrl}",
      s"-configuration-directory=${controller.environment.configDirectory.getPath}",
      (controller.environment.configDirectory / "b-scheduler.xml").getPath)
    new ExtraScheduler(
      args = args,
      tcpPort = Some(otherTcpPort),
      httpPort = Some(otherHttpPort))
      .registerCloseable
  }
  private lazy val commandClient = instance[HttpSchedulerCommandClient]
  private implicit lazy val actorSystem = instance[ActorSystem]
  private implicit lazy val entityManagerFactory = instance[EntityManagerFactory]

  override protected def checkedBeforeAll(): Unit = {
    databaseServer.start()
    super.checkedBeforeAll()
  }

  onClose {
    // Nach Scheduler-Ende
    databaseServer.close()
  }

  "Suspend order running in some other scheduler" in {
    awaitSuccess(bScheduler.start())
    executeCommandOnBScheduler(ModifyJobCommand(TestJobPath, cmd = Some(ModifyJobCommand.Cmd.Stop)))
    eventBus.awaitingKeyedEvent[OrderTouchedEvent](AOrderKey) {
      scheduler executeXml OrderCommand(AOrderKey).xmlElem
    }
    eventBus.awaitingKeyedEvent[OrderSuspendedEvent](AOrderKey) {
      try executeCommandOnBScheduler(ModifyOrderCommand(AOrderKey, suspended = Some(true)))
      catch {
        case e: Exception if e.getMessage startsWith s"$OrderIsOccupiedMessageCode " ⇒
        case e: Exception ⇒ throw e
      }
      eventBus.awaitingKeyedEvent[OrderStepEndedEvent](AOrderKey) {}
      transaction { implicit entityManager ⇒
        val entity = instance[HibernateOrderStore].fetch(AOrderKey)
        assert(entity.stateOption == Some(OrderState("200")))
        val e = SafeXML.loadString(entity.xmlOption.get)
        if (e \@ "suspended" != "yes") fail("Order should be suspended")
      }
    }
  }

  private def executeCommandOnBScheduler(xmlCommand: XmlCommand) =
    awaitSuccess(commandClient.execute(bScheduler.uri, xmlCommand.xmlElem))
}

private object JS1227IT {
  private val AOrderKey = JobChainPath("/test") orderKey "A"
  private val TestJobPath = JobPath("/test")
  private val OrderIsOccupiedMessageCode = MessageCode("SCHEDULER-379")
}
