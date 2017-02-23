package com.sos.scheduler.engine.test.binary

import com.sos.jobscheduler.data.log.SchedulerLogLevel
import com.sos.jobscheduler.data.scheduler.SchedulerId
import com.sos.scheduler.engine.main.CppBinaries
import java.io.File

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
    configurationFile: File)
