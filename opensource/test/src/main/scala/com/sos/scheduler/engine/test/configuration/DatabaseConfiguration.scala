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
  def testJdbcUrl(testName: String, directory: File): String
  def jdbcClassName: String
}


final case class DefaultDatabaseConfiguration(closeDelay: Duration = 0.s, autoServer: Boolean = false)
extends JdbcDatabaseConfiguration {
  def jdbcClassName = "org.h2.Driver"

  def testJdbcUrl(testName: String, directory: File) = {
    val suffix =
      (if (closeDelay == Duration.ZERO) "" else ";DB_CLOSE_DELAY=" + closeDelay.plus(999).getStandardSeconds) +
      (if (!autoServer) "" else ";AUTO_SERVER=TRUE")
    //s"jdbc:h2:mem:scheduler-$testName$suffix"
    s"jdbc:h2:$directory/database$suffix"
  }
}


object DefaultDatabaseConfiguration {
  val forJava = new DefaultDatabaseConfiguration
}


final case class HostwareDatabaseConfiguration(hostwareString: String)
extends DatabaseConfiguration
