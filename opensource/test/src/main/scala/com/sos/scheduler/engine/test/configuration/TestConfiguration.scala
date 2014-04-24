package com.sos.scheduler.engine.test.configuration

import com.sos.scheduler.engine.data.log.ErrorLogEvent
import com.sos.scheduler.engine.kernel.settings.{CppSettings, CppSettingName}
import com.sos.scheduler.engine.test.ResourceToFileTransformer
import com.sos.scheduler.engine.test.binary.CppBinariesDebugMode
import com.sos.scheduler.engine.data.message.MessageCode

final case class TestConfiguration(
    testClass: Class[_],

    /** Hier werden die Ressourcen für die Scheduler-Konfiguration erwartet: scheduler.xml, factory.ini, Jobs, usw. */
    testPackage: Option[Package] = None,

    resourceToFileTransformer: Option[ResourceToFileTransformer] = None,

    resourceNameMap: Iterable[(String, String)] = Nil,

    binariesDebugMode: Option[CppBinariesDebugMode] = None,

    /** Kommandozeilenparameter des C++-Codes. */
    mainArguments: Seq[String] = Seq(),

    /** Fürs scheduler.log */
    logCategories: String = "",

    database: Option[DatabaseConfiguration] = Some(DefaultDatabaseConfiguration()),

    /** Bricht den Test mit Fehler ab, wenn ein [[com.sos.scheduler.engine.data.log.ErrorLogEvent]] ausgelöst worden ist. */
    terminateOnError: Boolean = true,

    errorLogEventIsTolerated: ErrorLogEvent ⇒ Boolean = _ ⇒ false,

    ignoreError: MessageCode ⇒ Boolean = _ ⇒ false,

    cppSettings: Map[CppSettingName, String] = CppSettings.testMap)


object TestConfiguration {
  def of(testClass: Class[_]): TestConfiguration =
    new TestConfiguration(testClass = testClass)
}
