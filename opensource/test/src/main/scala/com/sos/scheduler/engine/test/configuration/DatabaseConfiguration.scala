package com.sos.scheduler.engine.test.configuration

import com.sos.scheduler.engine.common.time.ScalaJoda._
import org.joda.time.Duration

sealed trait DatabaseConfiguration

object DatabaseConfiguration {
  final case class JdbcClassAndUrl(className: String, url: String)
}

trait JdbcDatabaseConfiguration extends DatabaseConfiguration {
  def testJdbcUrl(testName: String): String
  def jdbcClassName: String
}

final case class DefaultDatabaseConfiguration(closeDelay: Duration = 0.s)
extends JdbcDatabaseConfiguration {
  def jdbcClassName = "org.h2.Driver"

  def testJdbcUrl(testName: String) = {
    val suffix = if (closeDelay == Duration.ZERO) "" else ";DB_CLOSE_DELAY=" + closeDelay.plus(999).getStandardSeconds
    s"jdbc:h2:mem:scheduler-$testName$suffix"
  }
}

object DefaultDatabaseConfiguration {
  val forJava = new DefaultDatabaseConfiguration
}


//final case class JdbcDatabaseConfiguration(jdbcClassName: String, jdbcUrl: String)
//extends DatabaseConfiguration {
//  def testJdbcUrl(testName: String) = jdbcUrl
//}


final case class HostwareDatabaseConfiguration(hostwareString: String)
extends DatabaseConfiguration
