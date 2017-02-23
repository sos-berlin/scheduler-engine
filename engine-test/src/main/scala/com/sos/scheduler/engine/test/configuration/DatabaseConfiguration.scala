package com.sos.scheduler.engine.test.configuration

import com.sos.jobscheduler.common.time.ScalaTime._
import java.io.File
import java.time.Duration

sealed trait DatabaseConfiguration

object DatabaseConfiguration {
  final case class JdbcClassAndUrl(className: String, url: String)
}

trait JdbcDatabaseConfiguration
extends DatabaseConfiguration {
  def testJdbcUrl(testName: String, directory: File): String
  def jdbcClassName: String
}

final case class DefaultDatabaseConfiguration(closeDelay: Duration = 0.s, autoServer: Boolean = false)
extends JdbcDatabaseConfiguration {
  def jdbcClassName = "org.h2.Driver"

  def testJdbcUrl(testName: String, directory: File) = {
    val suffix =
      (if (closeDelay == Duration.ZERO) "" else ";DB_CLOSE_DELAY=" + (closeDelay + 999.ms).getSeconds) +
      (if (!autoServer) "" else ";AUTO_SERVER=TRUE")
    //s"jdbc:h2:mem:scheduler-$testName$suffix"
    s"jdbc:h2:$directory/database$suffix"
  }
}

case object InMemoryDatabaseConfiguration
extends JdbcDatabaseConfiguration {
  def jdbcClassName = "org.h2.Driver"
  def testJdbcUrl(testName: String, directory: File) = s"jdbc:h2:mem:scheduler-$testName"
}

object DefaultDatabaseConfiguration {
  val forJava = new DefaultDatabaseConfiguration
}


final case class HostwareDatabaseConfiguration(hostwareString: String)
extends DatabaseConfiguration
