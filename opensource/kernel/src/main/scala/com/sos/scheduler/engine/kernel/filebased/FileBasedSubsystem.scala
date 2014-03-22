package com.sos.scheduler.engine.kernel.filebased

import com.sos.scheduler.engine.common.scalautil.ScalaUtils._
import com.sos.scheduler.engine.cplusplus.runtime.HasSister
import com.sos.scheduler.engine.data.filebased._
import com.sos.scheduler.engine.kernel.cppproxy.{File_basedC, SubsystemC}
import com.sos.scheduler.engine.kernel.scheduler.Subsystem
import scala.collection.JavaConversions._
import scala.collection.{immutable, mutable}
import scala.reflect.ClassTag

trait FileBasedSubsystem extends Subsystem {

  type MySubsystem <: FileBasedSubsystem
  type MyFileBased <: FileBased
  type MyFile_basedC <: File_basedC[MyFileBased] with HasSister[MyFileBased]
  type Path = MyFileBased#Path

  //implicit protected def schedulerThreadCallQueue: SchedulerThreadCallQueue

  val companion: FileBasedSubsystem.Companion[MySubsystem, Path, MyFileBased]

  private val mutablePathSet = new mutable.HashSet[Path] with mutable.SynchronizedSet[Path]

  def onFileBasedEvent(e: FileBasedEvent) {
    val path = e.typedPath.asInstanceOf[Path]
    assert(e.typedPath.fileBasedType == fileBasedType)
    assert(path.getClass == companion.pathClass, s"${path.getClass} is not expected ${companion.getClass}")
    e match {
      case e: FileBasedAddedEvent ⇒ mutablePathSet += path
      case e: FileBasedRemovedEvent ⇒ mutablePathSet -= path
      case _ =>
    }
  }

//  def fileBasedStateCounts: Map[FileBasedState, Int] =
//    inSchedulerThread { paths map { fileBased(_).fileBasedState } } .countEquals

  def count =
    paths.size

  def paths: Seq[Path] =
    mutablePathSet.toImmutableSeq

  def visiblePaths: Seq[Path] =
    cppProxy.file_based_paths(visibleOnly = true) map companion.stringToPath

  def fileBased(path: TypedPath): MyFileBased =
    cppProxy.java_file_based(path.string).getSister

  def fileBasedOption(path: TypedPath): Option[MyFileBased] =
    Option(cppProxy.java_file_based_or_null(path.string)) map { _.getSister }

  def fileBaseds: Seq[MyFileBased] =
    cppProxy.java_file_baseds

  def fileBasedType = companion.fileBasedType

  protected[this] def cppProxy: SubsystemC[MyFileBased, MyFile_basedC]
}


object FileBasedSubsystem {
  type AnyCompanion = Companion[_ <: FileBasedSubsystem, _ <: TypedPath, _ <: FileBased]

  abstract class Companion[S <: FileBasedSubsystem : ClassTag, P <: TypedPath : ClassTag, F <: FileBased](val fileBasedType: FileBasedType, val stringToPath: String ⇒ P) {
    type MyFileBased = F
    type Path = P
    val subsystemClass: Class[S] = implicitClass[S]
    val pathClass: Class[P] = implicitClass[P]
  }

  final class Register(val companions: immutable.Seq[AnyCompanion])
}
