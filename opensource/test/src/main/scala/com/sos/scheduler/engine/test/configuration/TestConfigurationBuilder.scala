package com.sos.scheduler.engine.test.configuration

import com.sos.scheduler.engine.test.ResourceToFileTransformer
import com.sos.scheduler.engine.test.binary.CppBinariesDebugMode
import scala.collection.JavaConversions._
import scala.collection.immutable

/** Für Java. */
final class TestConfigurationBuilder {
  private val default = TestConfiguration()
  private var _testPackage = default.testPackage
  private var _resourceToFileTransformer = default.resourceToFileTransformer
  private var _resourceNameMap = default.resourceNameMap
  private var _binariesDebugMode = default.binariesDebugMode
  private var _mainArguments = default.mainArguments
  private var _logCategories = default.logCategories
  private var _database = default.database
  private var _terminateOnError = default.terminateOnError

  def testPackage(o: Package) = {
    _testPackage = Some(o)
    this
  }

  def resourceToFileTransformer(o: ResourceToFileTransformer) = {
    _resourceToFileTransformer = Some(o)
    this
  }

  def resourceNameMap(o: java.util.Map[String, String]) = {
    _resourceNameMap = o.toMap
    this
  }

  def binariesDebugMode(o: CppBinariesDebugMode) = {
    _binariesDebugMode = Some(o)
    this
  }

  def mainArguments(o: java.lang.Iterable[String]) = {
    _mainArguments = immutable.Seq() ++ o
  }

  def logCategories(o: String) = {
    _logCategories = o
    this
  }

  def database(o: DatabaseConfiguration) = {
    _database = Some(o)
    this
  }

  def terminateOnError(o: Boolean) = {
    _terminateOnError = o
    this
  }

  def build: TestConfiguration = TestConfiguration(
    testPackage = _testPackage,
    resourceToFileTransformer = _resourceToFileTransformer,
    resourceNameMap = _resourceNameMap,
    binariesDebugMode = _binariesDebugMode,
    mainArguments = _mainArguments,
    logCategories = _logCategories,
    database = _database,
    terminateOnError= _terminateOnError
  )
}
