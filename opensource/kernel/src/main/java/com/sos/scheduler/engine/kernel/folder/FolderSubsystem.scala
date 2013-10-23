package com.sos.scheduler.engine.kernel.folder

import com.sos.scheduler.engine.data.folder.AbsolutePath
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.cppproxy.Folder_subsystemC
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue

final class FolderSubsystem(
    cppProxy: Folder_subsystemC,
    implicit private val schedulerThreadCallQueue: SchedulerThreadCallQueue)
extends FileBasedSubsystem {

  def names(path: AbsolutePath, typeName: String): Seq[String] = {
    val cppTypeName = typeName.replaceAll("([A-Z])", "_$1").toLowerCase
    cppProxy.java_names(path.asString, cppTypeName)
  }

  /** @return true, wenn ein [[com.sos.scheduler.engine.kernel.folder.FileBased]] geladen worden ist. */
  def updateFolders(): Boolean =
    inSchedulerThread {  // Im Scheduler-Thread, damit Events ordentlich verarbeitet werden
      cppProxy.update_folders_now()
    }
}
