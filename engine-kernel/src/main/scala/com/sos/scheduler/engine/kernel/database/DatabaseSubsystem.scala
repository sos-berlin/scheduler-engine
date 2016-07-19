package com.sos.scheduler.engine.kernel.database

import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.{HasCloser, SetOnce}
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.kernel.cppproxy.DatabaseC
import com.sos.scheduler.engine.kernel.scheduler.Subsystem
import com.sos.scheduler.engine.persistence.SchedulerDatabases.persistenceUnitName
import javax.persistence.Persistence.createEntityManagerFactory
import javax.persistence.{EntityManagerFactory, PersistenceException}
import scala.collection.JavaConversions._

@ForCpp
private[kernel] final class DatabaseSubsystem private[kernel](cppProxy: DatabaseC) extends Subsystem with HasCloser {

  private val entityManagerPropertiesOnce = new SetOnce[Map[String, String]]

  private[kernel] def newEntityManagerFactory(): EntityManagerFactory = {
    try createEntityManagerFactory(persistenceUnitName, entityManagerPropertiesOnce()) withCloser { _.close() }  // closes all EntityManager, too
    catch {
      // Hibernate provides only the message "Unable to build EntityManagerFactory" without the cause
      case e: PersistenceException ⇒ throw new RuntimeException(s"$e. Cause: ${e.getCause}", e)
    }
  }

  def onCppProxyInvalidated() = close()

  private[kernel] def onOpened(): Unit = {
    entityManagerPropertiesOnce :=  {
      val p = cppProxy.properties.getSister.toMap
      Map(
        //"hibernate.show_sql" → "true",
        "javax.persistence.jdbc.driver" → p("jdbc.driverClass"),
        "javax.persistence.jdbc.url" → p("path"),
        "javax.persistence.jdbc.user" → p("user"),
        "javax.persistence.jdbc.password" → p("password"))
    }
  }
}
