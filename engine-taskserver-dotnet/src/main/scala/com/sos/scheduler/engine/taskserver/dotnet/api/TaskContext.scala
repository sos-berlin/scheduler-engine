package com.sos.scheduler.engine.taskserver.dotnet.api

import com.sos.scheduler.engine.data.log.SchedulerLogLevel

/**
  * @author Joacim Zschimmer
  */
final case class TaskContext(
  spoolerLog: sos.spooler.Log,
  spoolerTask: sos.spooler.Task,
  spoolerJob: sos.spooler.Job,
  spooler: sos.spooler.Spooler,
  stderrLogLevel: SchedulerLogLevel)
