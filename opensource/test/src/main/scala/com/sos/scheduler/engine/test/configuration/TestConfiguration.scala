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
  nameMap: Map[String, String] = Map(),
  /** Kommandozeilenargumente für den Scheduler */
  mainArguments: Seq[String] = Seq(),
  logCategories: String = "",
  database: Option[DatabaseConfiguration] = None,
  /** Bricht den Test mit Fehler ab, wenn ein [[com.sos.scheduler.engine.data.log.ErrorLogEvent]] ausgelöst worden ist. */
  terminateOnError: Boolean = true,
  expectedErrorLogEventPredicate: ErrorLogEvent => Boolean = _ => false)
