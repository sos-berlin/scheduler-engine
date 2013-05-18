package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.data.log.ErrorLogEvent
import com.sos.scheduler.engine.test.binary.CppBinariesDebugMode

final case class TestConfiguration(
  testPackage: Option[Package] = None,
  resourceToFileTransformer: Option[ResourceToFileTransformer] = None,
  resourceNameMap: Iterable[Tuple2[String, String]] = Nil,
  binariesDebugMode: Option[CppBinariesDebugMode] = None,
  nameMap: Map[String, String] = Map(),
  mainArguments: Seq[String] = Seq(),
  logCategories: String = System.getProperty("scheduler.logCategories"),
  database: DatabaseConfiguration = DatabaseConfiguration(use = false),
  expectedErrorLogEventPredicate: ErrorLogEvent => Boolean = (_ => false)
) {
  //def javaResourceNameMap: ImmutableMap
}

object TestConfiguration {
  /** Für Java. */
  def standard =
    new TestConfiguration()
}