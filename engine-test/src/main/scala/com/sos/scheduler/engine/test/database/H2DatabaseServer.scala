package com.sos.scheduler.engine.test.database

import com.sos.jobscheduler.common.scalautil.FileUtils.implicits._
import com.sos.jobscheduler.common.scalautil.HasCloser
import com.sos.jobscheduler.common.scalautil.SideEffect._
import com.sos.scheduler.engine.test.configuration.JdbcDatabaseConfiguration
import java.io.File
import org.h2.tools.Server.createTcpServer

/**
 * @author Joacim Zschimmer
 */
final class H2DatabaseServer(configuration: H2DatabaseServer.Configuration) extends DatabaseServer with HasCloser {

  import configuration.{directory, tcpPort}

  private val name = s"database-$tcpPort"
  private val server = createTcpServer("-tcpPort", tcpPort.toString, "-baseDir", directory.getPath) sideEffect { o ⇒
    onClose { o.stop() }
  }

  def start(): Unit = server.start()

  def stop(): Unit = server.stop()

  protected[database] def file = directory / s"$name.mv.db"

  override def toString = s"H2DatabaseServer(${server.getStatus}})"
}

object H2DatabaseServer {
  trait Configuration extends JdbcDatabaseConfiguration {
    private val name = s"database-$tcpPort"
    lazy val jdbcUrl = s"jdbc:h2:tcp://127.0.0.1:$tcpPort/$directory/$name"

    def hostwarePath = s"jdbc -class=$jdbcClassName $jdbcUrl"
    def jdbcClassName = "org.h2.Driver"
    def testJdbcUrl(ignoredName: String, ignoredDirectory: File) = jdbcUrl

    def directory: File
    def tcpPort: Int
  }
}
