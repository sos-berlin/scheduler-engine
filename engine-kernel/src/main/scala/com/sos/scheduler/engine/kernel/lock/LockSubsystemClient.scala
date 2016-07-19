package com.sos.scheduler.engine.kernel.lock

import com.sos.scheduler.engine.data.lock.LockPath
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystemClient
import javax.inject.{Inject, Singleton}

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class LockSubsystemClient @Inject private(
  protected val subsystem: LockSubsystem)
  (implicit protected val schedulerThreadCallQueue: SchedulerThreadCallQueue)
extends FileBasedSubsystemClient {

  @deprecated("Avoid direct access to C++ near objects")
  def lock(path: LockPath) = fileBased(path)

  @deprecated("Avoid direct access to C++ near objects")
  def lockOption(path: LockPath) = fileBasedOption(path)
}
