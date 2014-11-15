package com.sos.scheduler.engine.tests.jira.js1176

import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.scalautil.SideEffect._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder.findRandomFreeTcpPort
import com.sos.scheduler.engine.test.configuration.{DatabaseConfiguration, JdbcDatabaseConfiguration}
import java.io.File
import org.h2.tools.Server.createTcpServer

/**
 * @param directory by-name, to be applicable to [[com.sos.scheduler.engine.test.TestEnvironment]].databaseDirectory
 *                  of the JobScheduler test framework
 * @author Joacim Zschimmer
 */
final class RestartableH2DatabaseServer(directory: ⇒ File) extends HasCloser {
  lazy val tcpPort = findRandomFreeTcpPort(FreeTcpPortFinder.alternateTcpPortRange)
  private lazy val databaseServer = createTcpServer("-tcpPort", tcpPort.toString, "-baseDir", directory.getPath) sideEffect { o ⇒
    onClose { o.stop() }
  }

  def start(): Unit = databaseServer.start()

  def stop(): Unit = databaseServer.stop()

  def databaseConfiguration: DatabaseConfiguration = new JdbcDatabaseConfiguration {
    def jdbcClassName = "org.h2.Driver"
    def testJdbcUrl(testName: String, directory: File) = s"jdbc:h2:tcp://localhost:$tcpPort/$directory/database-$tcpPort"
  }
}
