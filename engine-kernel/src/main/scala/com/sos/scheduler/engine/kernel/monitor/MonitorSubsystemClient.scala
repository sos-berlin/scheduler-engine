package com.sos.scheduler.engine.kernel.monitor

import com.sos.scheduler.engine.data.monitor.MonitorPath
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystemClient
import javax.inject.{Inject, Singleton}

/**
  * @author Joacim Zschimmer
  */
@Singleton
class MonitorSubsystemClient @Inject private(
  protected val subsystem: MonitorSubsystem)
  (implicit protected val schedulerThreadCallQueue: SchedulerThreadCallQueue)
extends FileBasedSubsystemClient {

  override type ThisPath = MonitorPath
  override type ThisFileBased = MonitorSister
}
