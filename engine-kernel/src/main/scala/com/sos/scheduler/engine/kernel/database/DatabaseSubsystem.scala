package com.sos.scheduler.engine.kernel.database

import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.{HasCloser, SetOnce}
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.kernel.cppproxy.DatabaseC
import com.sos.scheduler.engine.kernel.database.DatabaseSubsystem._
import com.sos.scheduler.engine.kernel.scheduler.Subsystem
import com.sos.scheduler.engine.persistence.SchedulerDatabases.persistenceUnitName
import java.io.Closeable
import java.sql
import java.time.Instant
import javax.persistence.Persistence.createEntityManagerFactory
import javax.persistence.{EntityManagerFactory, PersistenceException}
import scala.collection.JavaConversions._

@ForCpp
private[kernel] final class DatabaseSubsystem private[kernel](getCppProxy: () ⇒ DatabaseC)
extends Subsystem with HasCloser {

  private lazy val cppProxy = getCppProxy()
  private lazy val productName = cppProxy.jdbc_connection.asInstanceOf[sql.Connection].getMetaData.getDatabaseProductName
  private val cppPropertiesOnce = new SetOnce[CppDatabaseProperties]
  private val inClauseLimitOnce = new SetOnce[Int]

  private var databaseOpened = false

  private[database] def registerCloseable[A <: Closeable](o: A): A = closer.register(o)

  private[kernel] def onDatabaseOpened(): Unit = {
    databaseOpened = true
    cppPropertiesOnce := CppDatabaseProperties(cppProxy.properties.getSister.toMap)
    inClauseLimitOnce := (if (productName == "Oracle") 1000 else Int.MaxValue)
  }

  private[kernel] def cppProperties: CppDatabaseProperties = cppPropertiesOnce()

  private[kernel] def newEntityManagerFactory(): EntityManagerFactory = {
    require(databaseOpened, "EntityManagerFactory requested but before JobScheduler database has been opened")
    try createEntityManagerFactory(persistenceUnitName, cppProperties.toEntityManagerProperties) withCloser { _.close() }  // closes all EntityManager, too
    catch {
      // Hibernate provides only the message "Unable to build EntityManagerFactory" without the cause
      case e: PersistenceException ⇒ throw new RuntimeException(s"$e. Cause: ${e.getCause}", e)
    }
  }

  def onCppProxyInvalidated() = close()

  private[kernel] def toInClauseSql(column: String, sqlElements: TraversableOnce[String]): String =
    sqlElements.toSeq grouped inClauseLimitOnce() map {
      elements ⇒ quoteSqlName(column) + " in " + elements.mkString("(", ", ", ")")
    } mkString " or "

  private[kernel] def quoteSqlTimestamp(instant: Instant) =
    productName match {
      case "Oracle" =>
        val ts = new java.sql.Timestamp(instant.toEpochMilli)
        "{ts" + ts.toString + "}"
      case "H2" | _ =>
        quoteSqlString(instant.toString)
    }

  private[kernel] def transformSql(string: String) = cppProxy.transform_sql(string)
}

object DatabaseSubsystem
{
  def quoteSqlString(o: String) = {
    val quote = '\''
    require(!o.contains(quote), s"SQL string must not contain single quotes ($quote)")
    quote + o + quote
  }

  def quoteSqlName(o: String) = {
    val quote = '"'
    require(!o.contains(quote), s"SQL name must not contain double quotes ($quote)")
    quote + o + quote
  }
}
