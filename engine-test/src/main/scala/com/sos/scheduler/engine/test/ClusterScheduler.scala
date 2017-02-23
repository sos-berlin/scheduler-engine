package com.sos.scheduler.engine.test

import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.client.command.SchedulerClientFactory
import com.sos.scheduler.engine.data.xmlcommands.XmlCommand
import com.sos.scheduler.engine.kernel.extrascheduler.ExtraScheduler
import com.sos.scheduler.engine.main.CppBinary
import com.sos.scheduler.engine.test.database.H2DatabaseServer
import javax.inject.{Inject, Singleton}
import scala.concurrent.Future

/**
 * @author Joacim Zschimmer
 */
final class ClusterScheduler(
  testEnvironment: TestEnvironment,
  controller: TestSchedulerController,
  databaseConfiguration: H2DatabaseServer.Configuration,
  tcpPort: Int,
  httpPort: Int,
  schedulerClientFactory: SchedulerClientFactory)
extends AutoCloseable {

  private val extraScheduler = {
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
      s"-db=${databaseConfiguration.hostwarePath}",
      s"-config=${controller.environment.configDirectory / "scheduler.xml"}",
      s"-configuration-directory=${controller.environment.liveDirectory}",
      (controller.environment.configDirectory / "scheduler.xml").getPath)
    new ExtraScheduler(
      args = args,
      tcpPort = Some(tcpPort),
      httpPort = Some(httpPort))
  }

  /**
   * @return Future, successful when ClusterScheduler is active and therefore ready to use.
   */
  def start(): Future[Unit] = extraScheduler.start()

  def close(): Unit = extraScheduler.close()

  def postCommand(xmlCommand: XmlCommand): Future[String] =
    schedulerClientFactory.apply(extraScheduler.uri).execute(xmlCommand.xmlElem)

  def uri = extraScheduler.uri
}

private object ClusterScheduler {
  @Singleton
  final class Factory @Inject private(commandClient: SchedulerClientFactory) {
    def apply(
      testEnvironment: TestEnvironment,
      controller: TestSchedulerController,
      databaseConfiguration: H2DatabaseServer.Configuration,
      tcpPort: Int,
      httpPort: Int) =
    {
      new ClusterScheduler(testEnvironment, controller, databaseConfiguration, tcpPort, httpPort, commandClient)
    }
  }
}
