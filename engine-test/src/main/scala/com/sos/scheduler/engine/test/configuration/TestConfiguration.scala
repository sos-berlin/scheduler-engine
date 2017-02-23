package com.sos.scheduler.engine.test.configuration

import com.sos.jobscheduler.data.message.MessageCode
import com.sos.scheduler.engine.data.log.ErrorLogged
import com.sos.scheduler.engine.kernel.settings.{CppSettingName, CppSettings}
import com.sos.scheduler.engine.test.ResourceToFileTransformer
import com.sos.scheduler.engine.test.binary.CppBinariesDebugMode
import scala.language.existentials

final case class TestConfiguration(
  testClass: Class[_],

  /** Hier werden die Ressourcen für die Scheduler-Konfiguration erwartet: scheduler.xml, factory.ini, Jobs, usw. */
  testPackage: Option[Package] = None,

  resourceToFileTransformer: Option[ResourceToFileTransformer] = None,

  renameConfigurationFile: PartialFunction[String, String] = PartialFunction.empty,

  binariesDebugMode: Option[CppBinariesDebugMode] = None,

  /** Kommandozeilenparameter des C++-Codes. */
  mainArguments: Seq[String] = Seq(),

  /** Fürs scheduler.log */
  logCategories: String = "",

  database: Option[DatabaseConfiguration] = Some(DefaultDatabaseConfiguration()),

  /** Bricht den Test mit Fehler ab, wenn ein [[com.sos.scheduler.engine.data.log.ErrorLogged]] ausgelöst worden ist. */
  terminateOnError: Boolean = true,

  errorLoggedIsTolerated: ErrorLogged ⇒ Boolean = _ ⇒ false,

  ignoreError: MessageCode ⇒ Boolean = _ ⇒ false,

  cppSettings: Map[CppSettingName, String] = CppSettings.TestMap)


object TestConfiguration {
  def of(testClass: Class[_]): TestConfiguration =
    new TestConfiguration(testClass = testClass)
}
