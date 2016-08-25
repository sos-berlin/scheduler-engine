package com.sos.scheduler.engine.kernel.filebased

import com.sos.scheduler.engine.data.filebased.{FileBasedDetailed, FileBasedOverview}
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures._
import scala.collection.immutable

/**
  * @author Joacim Zschimmer
  */
trait FileBasedSubsystemClient {

  protected val subsystem: FileBasedSubsystem
  val companion = subsystem.companion

  type ThisPath = subsystem.Path
  type ThisFileBased = subsystem.companion.ThisFileBased

  protected[this] implicit def schedulerThreadCallQueue: SchedulerThreadCallQueue

  def forceFileReread(path: ThisPath): Unit =
    inSchedulerThread {
      subsystem.fileBased(path).forceFileReread()
    }

  def overview: FileBasedSubsystemOverview =
    inSchedulerThread {
      subsystem.overview
    }

  def detailed(path: ThisPath): FileBasedDetailed =
    inSchedulerThread {
      fileBasedDetailed(path)
    }

  def count = inSchedulerThread { subsystem.count }

  def contains(path: ThisPath): Boolean =
    inSchedulerThread {
      subsystem contains path
    }

  def visiblePaths: immutable.Seq[ThisPath] =
    inSchedulerThread {
      subsystem.visiblePaths.toVector
    }

  def paths: immutable.Seq[ThisPath] =
    inSchedulerThread {
      subsystem.paths
    }

  def fileBasedOverview(path: ThisPath): FileBasedOverview =
    inSchedulerThread {
      subsystem.fileBased(path).overview
    }

  def fileBasedDetailed(path: ThisPath): FileBasedDetailed =
    inSchedulerThread {
      subsystem.fileBased(path).details
    }

  @deprecated("Avoid direct access to C++ near objects")
  def fileBased(path: ThisPath) =
    inSchedulerThread {
      subsystem.fileBased(path)
    }

  @deprecated("Avoid direct access to C++ near objects")
  def fileBasedOption(path: ThisPath) =
    inSchedulerThread {
      subsystem.fileBasedOption(path)
    }

//  private def toClient(_fileBased: companion.ThisFileBased) =
//    _fileBased.clientOnce getOrUpdate {
//      new FileBasedClient {
//        type ThisFileBased = companion.ThisFileBased
//        def fileBased = _fileBased
//        def schedulerThreadCallQueue = FileBasedSubsystemClient.this.schedulerThreadCallQueue
//      }
//    }
}
