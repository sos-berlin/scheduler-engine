package com.sos.scheduler.engine.kernel.database

import com.sos.jobscheduler.common.concurrent.ThrottledExecutionContext
import com.sos.jobscheduler.common.scalautil.HasCloser
import com.sos.scheduler.engine.kernel.database.JdbcConnectionPool._
import com.typesafe.config.Config
import com.zaxxer.hikari.{HikariConfig, HikariDataSource}
import java.sql
import java.util.Properties
import scala.collection.JavaConversions._
import scala.collection.mutable
import scala.concurrent.{ExecutionContext, Future, blocking}

/**
  * @author Joacim Zschimmer
  */
final class JdbcConnectionPool(config: Config, cppPropertiesLazy: () ⇒ CppDatabaseProperties)(implicit ec: ExecutionContext)
extends HasCloser {

  private lazy val dataSource  = closer.register(newConnectionPool(config, cppPropertiesLazy()))  // Create connection pool only when needed
  // Good: Two implicits make ExecutionContext ambiguous, so we have to use it explicitly.
  private implicit lazy val throttledExecutionContext: ExecutionContext = new ThrottledExecutionContext(dataSource.getMaximumPoolSize)(ec)

  def maximumPoolSize = dataSource.getMaximumPoolSize

  def readOnly[A](body: sql.Connection ⇒ A): Future[A] =
    future { connection ⇒
      connection.setReadOnly(true)
      body(connection)
    }

  def future[A](body: sql.Connection ⇒ A): Future[A] =
    Future {
      blocking {
        val connection = dataSource.getConnection
        try body(connection)
        finally connection.close()
      }
    } (throttledExecutionContext)
}

object JdbcConnectionPool {
  private def newConnectionPool(globalConfig: Config, cppProperties: CppDatabaseProperties): HikariDataSource = {
    new HikariDataSource(toHikariConfig(globalConfig, cppProperties))
  }

  private def toHikariConfig(config: Config, cppProperties: CppDatabaseProperties) =
    // Additionally, system property "hikaricp.configurationFile" may designate a properties file with configuration defaults.
    new HikariConfig(toProperties(
      Map("autoCommit" → "false") ++
      configToStringMap(config.getConfig("hikari")) ++
      cppPropertiesToHikari(cppProperties)))

  // Exception, if an entry is not convertible to a string
  private def configToStringMap(config: Config): Map[String, String] =
    (for (e ← config.entrySet) yield e.getKey → config.getString(e.getKey))
    .toMap

  private def cppPropertiesToHikari(cppProperties: CppDatabaseProperties): Map[String, String] = {
    val result = mutable.Buffer[(String, String)]()
    result += "jdbcUrl" → cppProperties.url
    for (o ← cppProperties.driverClassName) result += "driverClassName" → o
    for (o ← cppProperties.user)            result += "username" → o
    for (o ← cppProperties.password)        result += "password" → o.string
    result.toMap
  }

  private def toProperties(map: TraversableOnce[(String, String)]): Properties = {
    val result = new Properties()
    for ((key, value) ← map) result.setProperty(key, value)
    result
  }
}
