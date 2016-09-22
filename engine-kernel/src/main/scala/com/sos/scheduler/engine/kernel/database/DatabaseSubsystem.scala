package com.sos.scheduler.engine.kernel.database

import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.{HasCloser, SetOnce}
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.kernel.cppproxy.DatabaseC
import com.sos.scheduler.engine.kernel.database.DatabaseSubsystem._
import com.sos.scheduler.engine.kernel.scheduler.Subsystem
import com.sos.scheduler.engine.persistence.SchedulerDatabases.persistenceUnitName
import java.sql.Connection
import javax.persistence.Persistence.createEntityManagerFactory
import javax.persistence.{EntityManagerFactory, PersistenceException}
import scala.collection.JavaConversions._

@ForCpp
private[kernel] final class DatabaseSubsystem private[kernel](getCppProxy: () ⇒ DatabaseC)
extends Subsystem with HasCloser {

  private lazy val cppProxy = getCppProxy()
  private lazy val cppProperties = cppProxy.properties.getSister.toMap
  private lazy val connection = cppProxy.jdbc_connection.asInstanceOf[Connection]
  private lazy val inClauseLimit = if (connection.getMetaData.getDatabaseProductName == "Oracle") 1000 else Int.MaxValue
  private var databaseOpened = false
  private val entityManagerPropertiesOnce = new SetOnce[Map[String, String]]

  private[kernel] def onDatabaseOpened(): Unit = {
    databaseOpened = true
  }

  private[kernel] def newEntityManagerFactory(): EntityManagerFactory = {
    require(databaseOpened, "EntityManagerFactory requested but before JobScheduler database has been opened")
    val properties = entityManagerPropertiesOnce getOrUpdate {
      Map(
        //"hibernate.show_sql" → "true",
        "javax.persistence.jdbc.driver"   → cppProperties("jdbc.driverClass"),
        "javax.persistence.jdbc.url"      → cppProperties("path"),
        "javax.persistence.jdbc.user"     → cppProperties("user"),
        "javax.persistence.jdbc.password" → cppProperties("password"))
    }
    try createEntityManagerFactory(persistenceUnitName, properties) withCloser { _.close() }  // closes all EntityManager, too
    catch {
      // Hibernate provides only the message "Unable to build EntityManagerFactory" without the cause
      case e: PersistenceException ⇒ throw new RuntimeException(s"$e. Cause: ${e.getCause}", e)
    }
  }

  def onCppProxyInvalidated() = close()

  private[kernel] def toInClauseSql(column: String, sqlElements: TraversableOnce[String]): String =
    sqlElements.toSeq grouped inClauseLimit map {
      elements ⇒ quoteSqlName(column) + " in " + elements.mkString("(", ", ", ")")
    } mkString " or "
}

object DatabaseSubsystem {
  def quoteSqlString(o: String) = {
    val quote = '\''
    require(!o.contains(quote), s"SQL string must not contain a single-quote ($quote)")
    quote + o + quote
  }

  def quoteSqlName(o: String) = {
    val quote = '`'
    require(!o.contains(quote), s"SQL name must not contain a back-tick ($quote)")
    quote + o + quote
  }
}
