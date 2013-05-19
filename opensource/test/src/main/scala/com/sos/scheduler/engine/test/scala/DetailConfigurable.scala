//package com.sos.scheduler.engine.test.scala
//
//import com.sos.scheduler.engine.test.binary.CppBinariesDebugMode
//import com.sos.scheduler.engine.test.{DatabaseConfiguration, TestConfiguration, ResourceToFileTransformer}
//
//trait DetailConfigurable {
//  this: ScalaSchedulerTest =>
//
//  protected val configurationPackage = getClass.getPackage
//  protected val schedulerResourceToFileTransformer: Option[ResourceToFileTransformer] = None
//  protected val schedulerResourceNameMap: Iterable[(String,String)] = List()
//  protected val binariesDebugMode: Option[CppBinariesDebugMode] = None
//  protected val arguments: Seq[String] = Nil
//  protected val logCategories: Option[String] = None
//  protected val databaseConfiguration = DatabaseConfiguration(use = false)
//
//  override lazy final val testConfiguration = TestConfiguration(
//    testPackage = Some(configurationPackage),
//    resourceToFileTransformer = schedulerResourceToFileTransformer,
//    binariesDebugMode = binariesDebugMode,
//    logCategories = logCategories getOrElse "",
//    nameMap = schedulerResourceNameMap.toMap)
//}
