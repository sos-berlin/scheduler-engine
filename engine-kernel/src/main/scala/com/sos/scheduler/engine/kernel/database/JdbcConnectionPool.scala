package com.sos.scheduler.engine.kernel.database

import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAny
import com.sos.scheduler.engine.common.scalautil.{HasCloser, Logger}
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.kernel.database.JdbcConnectionPool._
import com.zaxxer.hikari.{HikariConfig, HikariDataSource}
import java.sql
import java.util.concurrent.TimeUnit.MILLISECONDS
import java.util.concurrent.{LinkedBlockingQueue, ThreadPoolExecutor}
import scala.concurrent.{ExecutionContext, Future, Promise}
import scala.util.Try

/**
  * @author Joacim Zschimmer
  */
final class JdbcConnectionPool(cppPropertiesLazy: () ⇒ CppDatabaseProperties)
extends HasCloser {

  private lazy val (dataSource, sqlExecutionContext) = { // Create connection pool only when needed
    val dataSource = closer.register(newConnectionPool(cppPropertiesLazy()))
    val executorService = newThreadPoolExecutor(dataSource.getMaximumPoolSize) withCloser {
      _.shutdownNow()
    }
    val sqlExecutionContext = ExecutionContext.fromExecutorService(executorService, t ⇒ logger.warn(s"Future died with $t", t))
    (dataSource, sqlExecutionContext)
  }

  def poolSize = dataSource.getMaximumPoolSize

  def transactionFuture[A](body: sql.Connection ⇒ A): Future[A] = {
    implicit def ec = sqlExecutionContext
    val promise = Promise[A]()
    Future {
      val connection = dataSource.getConnection
      try promise complete Try { body(connection) }
      finally connection.close()  // After promise completion
    }
    promise.future
  }
}

object JdbcConnectionPool {
  private val logger = Logger(getClass)
  private val ThreadKeepAlive = 10.s

  private def newConnectionPool(cppProperties: CppDatabaseProperties): HikariDataSource = {
    // System property "hikaricp.configurationFile" may designate a properties file with configuration defaults.
    val config = new HikariConfig
    config.setJdbcUrl(cppProperties.url)
    for (o ← cppProperties.driverClassName) config.setDriverClassName(o)
    for (o ← cppProperties.user) config.setUsername(o)
    for (o ← cppProperties.password) config.setPassword(o.string)
    config.setAutoCommit(false)
    new HikariDataSource(config)
  }

  private def newThreadPoolExecutor(threadLimit: Int) = {
    val result = new ThreadPoolExecutor(  // Similar to Executors.newFixedThreadPool(threadLimit)
      threadLimit,
      threadLimit,
      ThreadKeepAlive.toMillis, MILLISECONDS,  // Long.MAX_VALUE TimeUnit.NANOSECONDS effectively disables idle threads from ever terminating prior to shut down.
      new LinkedBlockingQueue[Runnable])
    result.allowCoreThreadTimeOut(true)
    result
  }
}
