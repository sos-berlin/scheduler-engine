package com.sos.scheduler.engine.kernel.filebased

import com.sos.scheduler.engine.common.scalautil.ScalaUtils._
import com.sos.scheduler.engine.data.filebased._
import com.sos.scheduler.engine.kernel.scheduler.Subsystem
import scala.collection.{immutable, mutable}
import scala.reflect.ClassTag

trait FileBasedSubsystem extends Subsystem {

  type MyFileBased <: FileBased

  type Path = MyFileBased#Path

  def fileBasedType = companion.fileBasedType

  val companion: FileBasedSubsystem.AnyCompanion

  //implicit protected def schedulerThreadCallQueue: SchedulerThreadCallQueue

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

  def paths: immutable.Seq[Path] =
    mutablePathSet.toImmutableSeq

  def fileBased(o: Path): MyFileBased
}


object FileBasedSubsystem {
  type AnyCompanion = Companion[_ <: FileBasedSubsystem, _ <: TypedPath, _ <: FileBased]

  abstract class Companion[S <: FileBasedSubsystem : ClassTag, P <: TypedPath : ClassTag, F <: FileBased](val fileBasedType: FileBasedType) {
    type MyFileBased = F
    type Path = P
    val subsystemClass: Class[S] = implicitClass[S]
    val pathClass: Class[P] = implicitClass[P]
  }

  final class Register(val companions: immutable.Seq[AnyCompanion])
}
