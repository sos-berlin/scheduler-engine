package com.sos.scheduler.engine.tests.jira.js856

import JS856IT.modifiedParameters
import com.sos.scheduler.engine.test.configuration.{DefaultDatabaseConfiguration, TestConfiguration}
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner

/** JS-856 */
@RunWith(classOf[JUnitRunner])
final class DefaultJS856IT extends JS856IT("Current default behaviour: keep order state") {

  override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    database = Some(DefaultDatabaseConfiguration()),
    logCategories = "java.stackTrace-")   // Exceptions wegen fehlender Datenbanktabellen wollen wir nicht sehen.

  val finallyExpectedParameters = modifiedParameters
  val whenSuspendedExpectedParameters = modifiedParameters
}
