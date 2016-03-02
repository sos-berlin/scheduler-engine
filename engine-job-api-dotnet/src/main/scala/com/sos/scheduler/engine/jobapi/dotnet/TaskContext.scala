package com.sos.scheduler.engine.jobapi.dotnet

/**
  * @author Joacim Zschimmer
  */
final case class TaskContext(
  spoolerLog: sos.spooler.Log,
  spoolerTask: sos.spooler.Task,
  spoolerJob: sos.spooler.Job,
  spooler: sos.spooler.Spooler)
