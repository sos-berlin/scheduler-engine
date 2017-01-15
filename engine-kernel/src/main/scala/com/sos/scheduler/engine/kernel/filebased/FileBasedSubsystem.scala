package com.sos.scheduler.engine.kernel.filebased

import com.google.inject.Injector
import com.sos.scheduler.engine.base.utils.ScalaUtils.implicitClass
import com.sos.scheduler.engine.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.common.scalautil.Collections.implicits._
import com.sos.scheduler.engine.common.scalautil.ScalaConcurrentHashMap
import com.sos.scheduler.engine.cplusplus.runtime.HasSister
import com.sos.scheduler.engine.data.event.KeyedEvent
import com.sos.scheduler.engine.data.filebased._
import com.sos.scheduler.engine.data.filebaseds.TypedPathRegister
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.queries.PathQuery
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.cppproxy.{File_basedC, SubsystemC}
import com.sos.scheduler.engine.kernel.messagecode.MessageCodeHandler
import com.sos.scheduler.engine.kernel.scheduler.Subsystem
import scala.collection.{immutable, mutable}
import scala.reflect.ClassTag

trait FileBasedSubsystem extends Subsystem {
  self ⇒

  type ThisSubsystemClient <: FileBasedSubsystemClient
  type ThisSubsystem <: FileBasedSubsystem
  type Path <: TypedPath
  type ThisFileBased <: FileBased { type ThisPath = Path }
  type ThisFile_basedC <: File_basedC[ThisFileBased] with HasSister[ThisFileBased]

  protected def injector: Injector
  implicit val schedulerThreadCallQueue: SchedulerThreadCallQueue

  val companion: FileBasedSubsystem.AbstractCompanion[ThisSubsystemClient, ThisSubsystem, Path, ThisFileBased]
  private val pathOrdering: Ordering[Path] = Ordering by { _.string }

  private val _pathToFileBased = new ScalaConcurrentHashMap[Path, ThisFileBased]
  private val _orderedPaths = mutable.SortedSet[Path]()(pathOrdering)

  lazy val messageCodeHandler = injector.instance[MessageCodeHandler]

  private[kernel] def onFileBasedEvent(e: KeyedEvent[FileBasedEvent]): Unit = {
    val path = e.key.asInstanceOf[Path]
    assert(e.key.companion == companion.typedPathCompanion)
    assert(path.getClass == companion.pathClass, s"${path.getClass} is not expected ${companion.getClass}")
    e.event match {
      case FileBasedAdded ⇒
        _pathToFileBased += path → noCacheFileBased(path)
        _orderedPaths += path
      case FileBasedReplaced ⇒
        _pathToFileBased(path) = noCacheFileBased(path)
      case FileBasedRemoved ⇒
        _pathToFileBased -= path
        _orderedPaths -= path
      case _ ⇒
    }
  }

  private[kernel] def overview: FileBasedSubsystemOverview =
    SimpleFileBasedSubsystemOverview(
      fileBasedType = self.fileBasedType,
      count = self.count,
      fileBasedStateCounts = (fileBasedIterator map { _.fileBasedState }).countEquals)

  private[kernel] final def count: Int =
    _pathToFileBased.size

  private[kernel] final def contains(path: Path): Boolean =
    _pathToFileBased.get(path) exists { _.isVisible }

  private[kernel] final def paths: immutable.Seq[Path] = _pathToFileBased.keys.toVector

  private[kernel] def visiblePaths: Seq[Path] =
    cppProxy.file_based_paths(visibleOnly = true) map companion.stringToPath

  private[kernel] def orderedVisibleFileBasedIterator: Iterator[ThisFileBased] = orderedPaths map fileBased filter { _.isVisible }

  //private[kernel] def visibleFileBasedIterator: Iterator[ThisFileBased] = fileBasedIterator filter { _.isVisible }

  private[kernel] final def orderedPaths: Iterator[Path] = _orderedPaths.iterator

  private[kernel] final def requireExistence(path: Path): Unit = fileBased(path)

  private[kernel] final def fileBased(path: Path): ThisFileBased =
    _pathToFileBased.getOrElse(path, { throw new NoSuchElementException(messageCodeHandler(MessageCode("SCHEDULER-161"), fileBasedType, path.string)) })

  private final def noCacheFileBased(path: Path): ThisFileBased =
    fileBasedOption(path) getOrElse { throw new NoSuchElementException(messageCodeHandler(MessageCode("SCHEDULER-161"), fileBasedType, path.string))}

  private[kernel] final def fileBasedOption(path: Path): Option[ThisFileBased] =
    Option(cppProxy.java_file_based_or_null(path.string)) map { _.getSister }

  private[kernel] final def fileBasedsBy(query: PathQuery): Vector[ThisFileBased] = {
    (_pathToFileBased.values filter { o ⇒
      query.matches(o.path)(companion.typedPathCompanion)
    }).toVector
  }

  private[kernel] final def fileBaseds: Vector[ThisFileBased] =
    _pathToFileBased.values.toVector

  private[kernel] final def fileBasedIterator: Iterator[ThisFileBased] =
    _pathToFileBased.values.iterator

  final def fileBasedType = companion.fileBasedType

  protected[this] def cppProxy: SubsystemC[ThisFileBased, ThisFile_basedC]
}


object FileBasedSubsystem {

  trait Companion {
    type ThisFileBasedSubsystemClient <: FileBasedSubsystemClient
    type ThisFileBasedSubsystem <: FileBasedSubsystem
    type ThisFileBased <: FileBased
    type Path <: TypedPath

    val subsystemClass: Class[ThisFileBasedSubsystem]
    val clientClass: Class[ThisFileBasedSubsystemClient]
    val pathClass: Class[Path]
    val fileBasedType: FileBasedType
    def typedPathCompanion: TypedPath.Companion[Path] =
      TypedPathRegister.classToCompanion(pathClass)
    val stringToPath: String ⇒ Path

    override def toString = subsystemClass.getSimpleName
  }

  abstract class AbstractCompanion[
    C <: FileBasedSubsystemClient: ClassTag,
    S <: FileBasedSubsystem: ClassTag,
    P <: TypedPath: ClassTag,
    F <: FileBased]
  extends Companion {
    type ThisFileBasedSubsystemClient = C
    type ThisFileBasedSubsystem = S
    type ThisFileBased = F
    type Path = P
    val clientClass = implicitClass[ThisFileBasedSubsystemClient]
    val subsystemClass = implicitClass[ThisFileBasedSubsystem]
    val pathClass = implicitClass[Path]
  }

  final class Register private(injector: Injector, typedPathToCompanion: Map[TypedPath.AnyCompanion, Companion]) {
    val companions = typedPathToCompanion.values.toImmutableSeq

    private[kernel] def subsystem(t: FileBasedType): FileBasedSubsystem =
      injector.getInstance(typedPathToCompanion(TypedPathRegister.fileBasedTypedToCompanion(t)).subsystemClass)

    private[kernel] def fileBased[P <: TypedPath](path: P): FileBased = {
      val c = client(path.companion)
      c.fileBased(path.asInstanceOf[c.ThisPath])
    }

    private[kernel] def fileBaseds[P <: TypedPath: TypedPath.Companion](query: PathQuery): immutable.Seq[FileBased] = {
      val c = client(implicitly[TypedPath.Companion[P]])
      c.fileBaseds(query)
    }

//    def client[P <: TypedPath: TypedPath.Companion]: FileBasedSubsystemClient { type ThisPath = P } =
//      client(implicitly[TypedPath.Companion[P]])
//        .asInstanceOf[FileBasedSubsystemClient { type ThisPath = P }]

    def client(o: TypedPath.AnyCompanion): FileBasedSubsystemClient = client(typedPathToCompanion(o))

    def client(o: FileBasedSubsystem.Companion): FileBasedSubsystemClient = injector.getInstance(o.clientClass)
  }

  object Register {
    def apply(injector: Injector, descriptions: immutable.Seq[Companion]) =
      new Register(injector, descriptions toKeyedMap { _.typedPathCompanion })
  }
}
