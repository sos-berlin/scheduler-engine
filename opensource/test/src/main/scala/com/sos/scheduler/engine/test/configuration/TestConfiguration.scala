package com.sos.scheduler.engine.test.configuration

import com.sos.scheduler.engine.data.log.ErrorLogEvent
import com.sos.scheduler.engine.test.ResourceToFileTransformer
import com.sos.scheduler.engine.test.binary.CppBinariesDebugMode

final case class TestConfiguration(
    /** Hier werden die Ressourcen für die Scheduler-Konfiguration erwartet: scheduler.xml, factory.ini, Jobs, usw. */
    testPackage: Option[Package] = None,

    resourceToFileTransformer: Option[ResourceToFileTransformer] = None,

    resourceNameMap: Iterable[Tuple2[String, String]] = Nil,

    binariesDebugMode: Option[CppBinariesDebugMode] = None,

    /** Kommandozeilenparameter des C++-Codes. */
    mainArguments: Seq[String] = Seq(),

    /** Fürs scheduler.log */
    logCategories: String = "",

    database: Option[DatabaseConfiguration] = None,

    /** Bricht den Test mit Fehler ab, wenn ein [[com.sos.scheduler.engine.data.log.ErrorLogEvent]] ausgelöst worden ist. */
    terminateOnError: Boolean = true,

    errorLogEventIsExpected: ErrorLogEvent => Boolean = _ => false,

    ignoreError: String => Boolean = _ => false)
