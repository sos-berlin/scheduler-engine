package com.sos.scheduler.engine.kernel.folder

import com.sos.scheduler.engine.data.filebased.{FileBasedType, AbsolutePath}
import com.sos.scheduler.engine.data.folder.FolderPath
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.cppproxy.{FolderC, Folder_subsystemC}
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import javax.inject.{Inject, Singleton}
import scala.collection.immutable

@Singleton
final class FolderSubsystem @Inject private(
  protected[this] val cppProxy: Folder_subsystemC,
  implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue)
extends FileBasedSubsystem {

  type ThisSubsystem = FolderSubsystem
  type ThisFileBased = Folder
  type ThisFile_basedC = FolderC

  val description = FolderSubsystem

  def names(path: AbsolutePath, typeName: String): immutable.Seq[String] = {
    val cppTypeName = typeName.replaceAll("([A-Z])", "_$1").toLowerCase
    immutable.Seq() ++ cppProxy.java_names(path.string, cppTypeName)
  }

  /** @return true, wenn ein [[com.sos.scheduler.engine.kernel.filebased.FileBased]] geladen worden ist. */
  def updateFolders(): Boolean =
    inSchedulerThread {  // Im Scheduler-Thread, damit Events ordentlich verarbeitet werden
      cppProxy.update_folders_now()
    }
}


object FolderSubsystem extends FileBasedSubsystem.AbstractDesription[FolderSubsystem, FolderPath, Folder] {
  val fileBasedType = FileBasedType.folder
  val stringToPath = FolderPath.apply _
}