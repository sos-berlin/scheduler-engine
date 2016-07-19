package com.sos.scheduler.engine.kernel.processclass

import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystemClient
import javax.inject.{Inject, Singleton}

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class ProcessClassSubsystemClient @Inject private(
  protected val subsystem: ProcessClassSubsystem)
  (implicit protected val schedulerThreadCallQueue: SchedulerThreadCallQueue)
extends FileBasedSubsystemClient {

  def processClass(path: ProcessClassPath): ProcessClass = fileBased(path)

  def processClassOption(path: ProcessClassPath): Option[ProcessClass] = fileBasedOption(path)
}
