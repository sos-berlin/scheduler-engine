package com.sos.scheduler.engine.test.binary

import com.google.common.base.Strings._
import com.sos.scheduler.engine.common.system.OperatingSystem
import com.sos.scheduler.engine.common.system.OperatingSystem._
import com.sos.scheduler.engine.data.log.SchedulerLogLevel
import com.sos.scheduler.engine.data.scheduler.SchedulerId
import com.sos.scheduler.engine.main.{CppBinaries, CppBinary}
import com.sos.scheduler.engine.test.binary.SchedulerBinaryArguments._
import java.io.File
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
final case class SchedulerBinaryArguments(
    cppBinaries: CppBinaries,
    sosIniFile: File,
    iniFile: File,
    schedulerId: SchedulerId,
    logLevel: Option[SchedulerLogLevel],
    logDirectory: File,
    schedulerLog: String,
    environment: Map[String, String] = Map(),
    jobJavaOptions: String = "",
    configurationFile: File) {

  def toArgs: immutable.Seq[String] =
    Vector(cppBinaries.file(CppBinary.exeFilename).getPath) ++
    Some(s"-job-java-options=$jobJavaOptions") ++
    Some(s"-id=$schedulerId") ++
    Some(s"-sos.ini=$sosIniFile") ++
    Some(s"-ini=$iniFile") ++
    (logLevel map { o ⇒ s"-log-level=${o.cppName}" }) ++
    (if (schedulerLog.isEmpty) None else Some(s"-log=$schedulerLog")) ++
    (if (OperatingSystem.isUnix) Some("-env=" + libraryPathEnv(cppBinaries.directory)) else None)
}

private object SchedulerBinaryArguments {
  /** Damit der Scheduler die libspidermonkey.so aus seinem Programmverzeichnis laden kann. */
  private def libraryPathEnv(directory: File): String = {
    val varName = operatingSystem.getDynamicLibraryEnvironmentVariableName
    val previous = nullToEmpty(System.getenv(varName))
    s"$varName="+ OperatingSystem.concatFileAndPathChain(directory, previous)
  }
}
