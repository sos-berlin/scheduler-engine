package com.sos.scheduler.engine.kernel.database

import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.kernel.cppproxy.DatabaseC
import com.sos.scheduler.engine.kernel.scheduler.Subsystem
import com.sos.scheduler.engine.persistence.SchedulerDatabases.persistenceUnitName
import javax.persistence.Persistence.createEntityManagerFactory
import javax.persistence.{EntityManagerFactory, PersistenceException}
import scala.collection.JavaConversions._

@ForCpp
final class DatabaseSubsystem private[kernel](cppProxy: DatabaseC) extends Subsystem with HasCloser {

  private lazy val entityManagerProperties: Map[String, String] = {
    val p = cppProxy.properties.getSister.toMap
    Map(
      //"hibernate.show_sql" → "true",
      "javax.persistence.jdbc.driver" → p("jdbc.driverClass"),
      "javax.persistence.jdbc.url" → p("path"),
      "javax.persistence.jdbc.user" → p("user"),
      "javax.persistence.jdbc.password" → p("password"))
  }

  lazy val entityManagerFactory: EntityManagerFactory = {
    try createEntityManagerFactory(persistenceUnitName, entityManagerProperties) withCloser { _.close() }  // closes all EntityManager, too
    catch {
      // Hibernate provides only the message "Unable to build EntityManagerFactory" without the cause
      case e: PersistenceException ⇒ throw new RuntimeException(s"$e. Cause: ${e.getCause}", e)
    }
  }

  def onCppProxyInvalidated() = close()

  private[kernel] def onOpened(): Unit = {
    entityManagerProperties
  }
}
