package com.sos.scheduler.engine.test

import TestEnvironment._
import _root_.scala.collection.immutable
import com.google.common.base.Strings.nullToEmpty
import com.google.common.io.Files
import com.sos.scheduler.engine.common.scalautil.SideEffect._
import com.sos.scheduler.engine.common.system.Files.{makeDirectories, makeDirectory}
import com.sos.scheduler.engine.common.system.OperatingSystem
import com.sos.scheduler.engine.common.system.OperatingSystem.operatingSystem
import com.sos.scheduler.engine.data.folder.{JobPath, TypedPath}
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.data.scheduler.SchedulerId
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.schedulerEncoding
import com.sos.scheduler.engine.kernel.util.ResourcePath
import com.sos.scheduler.engine.main.CppBinaries
import com.sos.scheduler.engine.main.CppBinary
import java.io.File

/** Build the environment for the scheduler binary. */
final class TestEnvironment(
    resourcePath: ResourcePath,
    val directory: File,
    nameMap: Map[String, String],
    fileTransformer: ResourceToFileTransformer) {

  val configDirectory = new File(directory, configSubdir)
  val liveDirectory = configDirectory
  val logDirectory = directory
  val schedulerLog = new File(logDirectory, "scheduler.log")

  private[test] def prepare() {
    prepareTemporaryConfigurationDirectory()
  }

  private def prepareTemporaryConfigurationDirectory() {
    makeDirectories(directory)
    makeDirectories(configDirectory)
    makeDirectories(logDirectory)
    TestEnvironmentFiles.copy(resourcePath, configDirectory, nameMap, fileTransformer)
  }

  private[test] def standardArgs(cppBinaries: CppBinaries, logCategories: String): immutable.Iterable[String] =
    List(
      cppBinaries.file(CppBinary.exeFilename).getPath,
      s"-job-java-options=$jobJavaOptions",
      s"-id=$schedulerId",
      s"-sos.ini=$sosIniFile",
      s"-ini=$iniFile",
      s"-log-level=debug9",
      s"-log-dir=${logDirectory.getPath}",
      s"-log=$logCategories> ${schedulerLog.getPath}") ++
    (if (OperatingSystem.isUnix) Some("-env=" + libraryPathEnv(cppBinaries.directory)) else None) ++
    Some(configDirectory.getPath)

  def sosIniFile =
    new File(configDirectory, "sos.ini").getAbsoluteFile

  def iniFile =
    new File(configDirectory, "factory.ini").getAbsoluteFile

  def fileFromPath(p: TypedPath) =
    p.file(configDirectory)

  /** @return Pfad der Auftragsprotokolldatei für einfache OrderKey */
  def orderLogFile(orderKey: OrderKey) =
    new File(logDirectory, s"order.${orderKey.jobChainPath.withoutStartingSlash}.${orderKey.id.string}.log")

  def taskLogFileString(jobPath: JobPath): String =
    Files.toString(taskLogFile(jobPath), schedulerEncoding)

  /** @return Pfad einer Task-Potokolldatei für einfachen JobPath. */
  def taskLogFile(jobPath: JobPath) =
    new File(logDirectory, s"task.${jobPath.withoutStartingSlash}.log")

  def subdirectory(name: String) =
    new File(directory, name) sideEffect makeDirectory
}

object TestEnvironment {
  val schedulerId = new SchedulerId("test")
  private val jobJavaOptions = "-Xms5m -Xmx10m"
  private val configSubdir = "config"

  /** Damit der Scheduler die libspidermonkey.so aus seinem Programmverzeichnis laden kann. */
  private def libraryPathEnv(directory: File): String = {
    val varName = operatingSystem.getDynamicLibraryEnvironmentVariableName
    val previous = nullToEmpty(System.getenv(varName))
    s"$varName="+ OperatingSystem.concatFileAndPathChain(directory, previous)
  }
}
