package com.sos.scheduler.engine.kernel.folder

import com.google.inject.Injector
import com.sos.jobscheduler.data.folder.FolderPath
import com.sos.scheduler.engine.data.filebased.FileBasedType
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.cppproxy.{FolderC, Folder_subsystemC}
import com.sos.scheduler.engine.kernel.filebased.FileBasedSubsystem
import javax.inject.{Inject, Singleton}
import scala.collection.immutable

@Singleton
final class FolderSubsystem @Inject private(
  protected[this] val cppProxy: Folder_subsystemC,
  implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue,
  protected val injector: Injector)
extends FileBasedSubsystem {

  type ThisSubsystemClient = FolderSubsystemClient
  type ThisSubsystem = FolderSubsystem
  type ThisFileBased = Folder
  type ThisFile_basedC = FolderC
  type Path = FolderPath

  val companion = FolderSubsystem

  def names(path: FolderPath, typ: FileBasedType): immutable.Seq[String] =
    inSchedulerThread {
      immutable.Seq() ++ cppProxy.java_names(path.string, typ.cppName)
    }

  /** @return true, wenn ein [[com.sos.scheduler.engine.kernel.filebased.FileBased]] geladen worden ist. */
  private[kernel] def updateFolders(): Boolean =
    inSchedulerThread {  // Im Scheduler-Thread, damit Events ordentlich verarbeitet werden
      cppProxy.update_folders_now()
    }
}


object FolderSubsystem
extends FileBasedSubsystem.AbstractCompanion[FolderSubsystemClient, FolderSubsystem, FolderPath, Folder] {

  val fileBasedType = FileBasedType.Folder
  val stringToPath = FolderPath.apply _
}
