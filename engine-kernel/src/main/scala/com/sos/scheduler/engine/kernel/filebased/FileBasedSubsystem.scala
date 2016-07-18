package com.sos.scheduler.engine.kernel.filebased

import com.google.inject.{Guice, Injector}
import com.sos.scheduler.engine.base.utils.ScalaUtils.implicitClass
import com.sos.scheduler.engine.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.common.scalautil.Collections.implicits._
import com.sos.scheduler.engine.common.scalautil.ScalaConcurrentHashMap
import com.sos.scheduler.engine.cplusplus.runtime.HasSister
import com.sos.scheduler.engine.data.filebased._
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.cppproxy.{File_basedC, SubsystemC}
import com.sos.scheduler.engine.kernel.messagecode.MessageCodeHandler
import com.sos.scheduler.engine.kernel.scheduler.Subsystem
import scala.collection.immutable
import scala.reflect.ClassTag

trait FileBasedSubsystem extends Subsystem {
  self ⇒

  type ThisSubsystem <: FileBasedSubsystem
  type ThisFileBased <: FileBased
  type ThisFile_basedC <: File_basedC[ThisFileBased] with HasSister[ThisFileBased]
  type Path = ThisFileBased#ThisPath

  protected def injector: Injector
  implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue

  val description: FileBasedSubsystem.AbstractDesription[ThisSubsystem, Path, ThisFileBased]
  private val _pathToFileBased = new ScalaConcurrentHashMap[Path, ThisFileBased]

  lazy val messageCodeHandler = injector.instance[MessageCodeHandler]

  def onFileBasedEvent(e: FileBasedEvent, fileBased: FileBased): Unit = {
    val path = e.typedPath.asInstanceOf[Path]
    assert(e.typedPath.fileBasedType == fileBasedType)
    assert(fileBased.fileBasedType == fileBasedType)
    assert(path.getClass == description.pathClass, s"${path.getClass} is not expected ${description.getClass}")
    e match {
      case e: FileBasedAddedEvent ⇒ _pathToFileBased += path → fileBased.asInstanceOf[ThisFileBased]
      case e: FileBasedReplacedEvent ⇒ _pathToFileBased(path) = fileBased.asInstanceOf[ThisFileBased]
      case e: FileBasedRemovedEvent ⇒ _pathToFileBased -= path
      case _ ⇒
    }
  }

  def overview: FileBasedSubsystemOverview =
    inSchedulerThread {
      SimpleFileBasedSubsystemOverview(
        fileBasedType = self.fileBasedType,
        count = self.count,
        fileBasedStateCounts = (fileBaseds map { _.fileBasedState }).countEquals)
    }

  final def count: Int =
    _pathToFileBased.size

  final def contains(path: Path): Boolean =
    _pathToFileBased.get(path) exists { _.isVisible }

  final def paths: immutable.Seq[Path] = _pathToFileBased.keys.toVector

  def visiblePaths: Seq[Path] =
    cppProxy.file_based_paths(visibleOnly = true) map description.stringToPath

  def visibleFileBaseds: Vector[ThisFileBased] = fileBaseds filter { _.isVisible }

  final def fileBased(path: Path): ThisFileBased =
    _pathToFileBased.getOrElse(path, {  throw new NoSuchElementException(messageCodeHandler(MessageCode("SCHEDULER-161"), fileBasedType, path.string)) })

  final def fileBasedOption(path: Path): Option[ThisFileBased] =
    Option(cppProxy.java_file_based_or_null(path.string)) map { _.getSister }

  final def fileBaseds: Vector[ThisFileBased] =
    _pathToFileBased.values.toVector

  final def fileBasedType = description.fileBasedType

  protected[this] def cppProxy: SubsystemC[ThisFileBased, ThisFile_basedC]
}


object FileBasedSubsystem {

  trait Description {
    type ThisFileBasedSubsystem <: FileBasedSubsystem
    type ThisFileBased <: FileBased
    type Path <: TypedPath

    val subsystemClass: Class[ThisFileBasedSubsystem]
    val pathClass: Class[Path]
    val fileBasedType: FileBasedType
    val stringToPath: String ⇒ Path

    override def toString = subsystemClass.getSimpleName
  }

  abstract class AbstractDesription[S <: FileBasedSubsystem : ClassTag, P <: TypedPath : ClassTag, F <: FileBased]
  extends Description {
    type ThisFileBasedSubsystem = S
    type ThisFileBased = F
    type Path = P
    val subsystemClass = implicitClass[ThisFileBasedSubsystem]
    val pathClass = implicitClass[Path]
  }

  final class Register private(injector: Injector, typeToDescription: Map[FileBasedType, Description]) {
    val descriptions = typeToDescription.values.toImmutableSeq
    def subsystem(t: FileBasedType): FileBasedSubsystem = injector.getInstance(description(t).subsystemClass)
    def description(t: FileBasedType): Description = typeToDescription(t)
  }

  object Register {
    def apply(injector: Injector, descriptions: immutable.Seq[Description]) =
      new Register(injector, descriptions toKeyedMap { _.fileBasedType })
  }
}
