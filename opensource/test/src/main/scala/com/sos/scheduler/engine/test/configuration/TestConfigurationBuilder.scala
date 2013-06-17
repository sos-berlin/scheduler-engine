package com.sos.scheduler.engine.test.configuration

import com.sos.scheduler.engine.test.ResourceToFileTransformer
import com.sos.scheduler.engine.test.binary.CppBinariesDebugMode
import scala.collection.JavaConversions._
import scala.collection.immutable

/** Für Java. */
class TestConfigurationBuilder {
  private val default = TestConfiguration()
  private var _testPackage = default.testPackage
  private var _resourceToFileTransformer = default.resourceToFileTransformer
  private var _resourceNameMap = default.resourceNameMap
  private var _binariesDebugMode = default.binariesDebugMode
  private var _nameMap= default.nameMap
  private var _mainArguments = default.mainArguments
  private var _logCategories = default.logCategories
  private var _database = default.database
  private var _terminateOnError = default.terminateOnError
  //var expectedErrorLogEventPredicate: ErrorLogEvent => Boolean = (_ => false)

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


  def nameMap(o: java.util.Map[String, String]) = {
    _nameMap = o.toMap
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
    nameMap = _nameMap,
    mainArguments = _mainArguments,
    logCategories = _logCategories,
    database = _database,
    //expectedErrorLogEventPredicate = _expectedErrorLogEventPredicate,
    terminateOnError= _terminateOnError
  )
}
