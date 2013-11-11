package com.sos.scheduler.engine.test.configuration

import com.sos.scheduler.engine.common.time.ScalaJoda._
import java.io.File
import org.joda.time.Duration

sealed trait DatabaseConfiguration

object DatabaseConfiguration {
  final case class JdbcClassAndUrl(className: String, url: String)
}


trait JdbcDatabaseConfiguration
extends DatabaseConfiguration {
  def testJdbcUrl(testName: String): String
  def jdbcClassName: String
}


final case class DefaultDatabaseConfiguration(closeDelay: Duration = 0.s, autoServer: Boolean = false)
extends JdbcDatabaseConfiguration {
  def jdbcClassName = "org.h2.Driver"

  def testJdbcUrl(testName: String) = {
    val suffix =
      (if (closeDelay == Duration.ZERO) "" else ";DB_CLOSE_DELAY=" + closeDelay.plus(999).getStandardSeconds) +
      (if (!autoServer) "" else ";AUTO_SERVER=TRUE")
    s"jdbc:h2:mem:scheduler-$testName$suffix"
  }
}

object DefaultDatabaseConfiguration {
  val forJava = new DefaultDatabaseConfiguration
}


final case class ServerDatabaseConfiguration()
extends JdbcDatabaseConfiguration {
  private lazy val pathPrefix = new File(System.getProperty("java.io.tmpdir"), "test-h2database-").getPath

  def jdbcClassName = "org.h2.Driver"

  def testJdbcUrl(testName: String) = {
    val dbName = s"$pathPrefix$testName"
    deleteH2DatabaseFiles(dbName)
    s"jdbc:h2:$dbName;AUTO_SERVER=TRUE"
  }
  
  private def deleteH2DatabaseFiles(dbName: String) {
    for (o <- List(".h2.db", ".trace.db")) new File(s"$dbName$o").delete()
  }
}


final case class HostwareDatabaseConfiguration(hostwareString: String)
extends DatabaseConfiguration
