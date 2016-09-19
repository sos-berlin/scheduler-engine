package com.sos.scheduler.engine.kernel.folder

import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystemClient
import javax.inject.{Inject, Singleton}

/**
  * @author Joacim Zschimmer
  */
@Singleton
final class FolderSubsystemClient @Inject private(
  protected val subsystem: FolderSubsystem)
  (implicit protected val schedulerThreadCallQueue: SchedulerThreadCallQueue)
extends FileBasedSubsystemClient {

  def updateFolders(): Unit =
    inSchedulerThread {
      subsystem.updateFolders()
    }
}
