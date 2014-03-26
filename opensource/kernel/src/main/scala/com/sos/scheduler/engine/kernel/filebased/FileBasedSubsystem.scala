package com.sos.scheduler.engine.kernel.filebased

import com.google.inject.Injector
import com.sos.scheduler.engine.common.scalautil.ScalaUtils._
import com.sos.scheduler.engine.cplusplus.runtime.HasSister
import com.sos.scheduler.engine.data.filebased._
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.cppproxy.{File_basedC, SubsystemC}
import com.sos.scheduler.engine.kernel.scheduler.Subsystem
import scala.collection.JavaConversions._
import scala.collection.{immutable, mutable}
import scala.reflect.ClassTag

trait FileBasedSubsystem extends Subsystem {
  self ⇒

  type MySubsystem <: FileBasedSubsystem
  type MyFileBased <: FileBased
  type MyFile_basedC <: File_basedC[MyFileBased] with HasSister[MyFileBased]
  type Path = MyFileBased#Path

  val companion: FileBasedSubsystem.Companion[MySubsystem, Path, MyFileBased]

  implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue

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

  def overview: FileBasedSubsystemOverview =
    inSchedulerThread {
      SimpleFileBasedSubsystemOverview(
        fileBasedType = self.fileBasedType,
        count = self.count,
        fileBasedStateCounts = (fileBaseds map { _.fileBasedState }).countEquals)
    }

  final def count =
    paths.size

  final def paths: Seq[Path] =
    mutablePathSet.toImmutableSeq

  def visiblePaths: Seq[Path] =
    cppProxy.file_based_paths(visibleOnly = true) map companion.stringToPath

  final def fileBased(path: Path): MyFileBased =
    cppProxy.java_file_based(path.string).getSister

  final def fileBasedOption(path: Path): Option[MyFileBased] =
    Option(cppProxy.java_file_based_or_null(path.string)) map { _.getSister }

  final def fileBaseds: Seq[MyFileBased] =
    cppProxy.java_file_baseds

  final def fileBasedType = companion.fileBasedType

  protected[this] def cppProxy: SubsystemC[MyFileBased, MyFile_basedC]
}


object FileBasedSubsystem {
  type AnyCompanion = Companion[_ <: FileBasedSubsystem, _ <: TypedPath, _ <: FileBased]

  abstract class Companion[S <: FileBasedSubsystem : ClassTag, P <: TypedPath : ClassTag, F <: FileBased](val fileBasedType: FileBasedType, val stringToPath: String ⇒ P) {
    type MyFileBased = F
    type Path = P
    val subsystemClass: Class[S] = implicitClass[S]
    val pathClass: Class[P] = implicitClass[P]
    override def toString = subsystemClass.getSimpleName
  }

  final class Register private(injector: Injector, typeToCompanion: Map[FileBasedType, AnyCompanion]) {
    val companions = typeToCompanion.values.toImmutableSeq
    def subsystem(cppName: String): FileBasedSubsystem = subsystem(FileBasedType fromCppName cppName)
    def subsystem(t: FileBasedType): FileBasedSubsystem = injector.getInstance(companion(t).subsystemClass)
    def companion(t: FileBasedType) = typeToCompanion(t)
  }

  object Register {
    def apply(injector: Injector, companions: immutable.Seq[AnyCompanion]) =
      new Register(injector, companions toKeyedMap { _.fileBasedType })
  }
}
