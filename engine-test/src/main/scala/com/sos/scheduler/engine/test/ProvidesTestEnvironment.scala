package com.sos.scheduler.engine.test

import com.sos.jobscheduler.common.scalautil.Closers.implicits._
import com.sos.jobscheduler.common.time.ScalaTime._
import com.sos.scheduler.engine.persistence.SchedulerDatabases.persistenceUnitName
import com.sos.scheduler.engine.test.configuration.{JdbcDatabaseConfiguration, TestConfiguration}
import javax.persistence.Persistence.createEntityManagerFactory
import javax.persistence.PersistenceException
import scala.collection.JavaConversions._

trait ProvidesTestEnvironment extends ProvidesTestDirectory {

  final def testClass =
    testConfiguration.testClass

  protected def testConfiguration: TestConfiguration

  lazy val testEnvironment =
    TestEnvironment(testConfiguration, testDirectory).closeWithCloser

  private lazy val entityManagerFactory = {
    val properties: Map[String, String] =
      testConfiguration.database match {
        case c: JdbcDatabaseConfiguration => Map(
          "javax.persistence.jdbc.driver" -> c.jdbcClassName,
          "javax.persistence.jdbc.url" -> c.testJdbcUrl(testName, testEnvironment.databaseDirectory))
        case c => sys.error("No JdbcDatabaseConfiguration")
      }

      try createEntityManagerFactory(persistenceUnitName, properties)
      catch {
        case e: PersistenceException => throw new RuntimeException(s"$e. Cause: ${e.getCause}", e)  // Hibernate liefert nur nichtssagende Meldung "Unable to build EntityManagerFactory", ohne den interessanten Cause
      }
  }

  def newTestSchedulerController() =
    TestSchedulerController(testConfiguration, testEnvironment)

  def runScheduler[A](activate: Boolean = true)(f: TestSchedulerController => A): A = {
    val controller = newTestSchedulerController()
    try {
      if (activate)
        controller.activateScheduler()
      f(controller)
    } finally
      if (controller.isStarted) {
        controller.terminateScheduler()
        try controller.waitForTermination(3.h)
        finally controller.close()
      }
  }
}

object ProvidesTestEnvironment {
//  def runScheduler[A](testConfiguration: TestConfiguration, activate: Boolean = true)(f: TestSchedulerController => A): A =
//    autoClosing(ProvidesTestEnvironment(testConfiguration)) { e =>
//      e.runScheduler(activate = activate)(f)
//    }

  def apply(testConfiguration: TestConfiguration) = {
    val conf = testConfiguration
    new ProvidesTestEnvironment {
      override lazy val testConfiguration = conf
    }
  }
}
