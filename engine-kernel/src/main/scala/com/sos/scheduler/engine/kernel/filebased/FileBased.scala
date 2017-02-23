package com.sos.scheduler.engine.kernel.filebased

import com.sos.jobscheduler.common.scalautil.Collections.emptyToNone
import com.sos.jobscheduler.common.scalautil.SetOnce
import com.sos.jobscheduler.common.xml.XmlUtils.xmlBytesToString
import com.sos.jobscheduler.data.filebased._
import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.filebased.{FileBasedDetailed, FileBasedObstacle, FileBasedOverview, FileBasedState, FileBasedView}
import com.sos.scheduler.engine.data.filebaseds.TypedPathRegister
import com.sos.scheduler.engine.eventbus.EventSource
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.kernel.async.SchedulerThreadFutures.inSchedulerThread
import com.sos.scheduler.engine.kernel.cppproxy.File_basedC
import com.sos.scheduler.engine.kernel.log.PrefixLog
import java.nio.file.{Path, Paths}
import java.time.Instant
import scala.util.control.NonFatal

@ForCpp
trait FileBased
extends Sister
with EventSource {

  protected type Self <: FileBased
  type ThisPath <: TypedPath

  private val _fixedPath = new SetOnce[ThisPath]

  protected[kernel] def subsystem: FileBasedSubsystem

  protected implicit def schedulerThreadCallQueue: SchedulerThreadCallQueue = subsystem.schedulerThreadCallQueue

  //private[kernel] val clientOnce = new SetOnce[FileBasedClient]

  /** Jedes Exemplar hat seine eigene UUID. */
  final val uuid = java.util.UUID.randomUUID

  protected[this] def cppProxy: File_basedC[_]

  private[kernel] def fileBasedView[V <: FileBasedView: FileBasedView.Companion]: V =
    implicitly[FileBasedView.Companion[V]] match {
      case FileBasedOverview ⇒ fileBasedOverview
      case FileBasedDetailed ⇒ fileBasedDetailed
    }

  private[kernel] def fileBasedDetailed: FileBasedDetailed = {
    val overview = this.fileBasedOverview
    FileBasedDetailed(
      overview = overview,
      file = fileOption,
      fileModifiedAt = fileModificationInstantOption,
      sourceXml = sourceXmlBytes match {
        case o if o.isEmpty ⇒ None
        case o ⇒
          try Some(xmlBytesToString(o))
          catch { case NonFatal(t) ⇒ Some(<ERROR>{t.toString}</ERROR>.toString()) }
      })
    }

  private[kernel] def fileBasedOverview: FileBasedOverview =
    inSchedulerThread {
      FileBasedOverview(
        path = pathOrKey,
        fileBasedState = fileBasedState)
    }

  private[kernel] final def fileBasedObstacles: Set[FileBasedObstacle] = {
    import FileBasedObstacle._
    val builder = Set.newBuilder[FileBasedObstacle]
    fileBasedState match {
      case FileBasedState.active ⇒
      case FileBasedState.not_initialized ⇒  // ad-hoc objects
      case o ⇒ builder += BadState(o, error = fileBasedErrorMessageOption map { _
        .stripPrefix("Z-JAVA-105  Java exception ")
        .stripSuffix(", method=CallObjectMethodA []")
        .trim })
    }
    replacementOption match {
      case Some(replacement) ⇒
        builder += Replaced(replacement.fileBasedErrorMessageOption)
      case None if configurationFileRemoved ⇒
        Some(Removed)
      case _ ⇒
    }
    val missingRequisites = this.missingRequisites
    if (missingRequisites.nonEmpty) {
      builder += MissingRequisites(missingRequisites)
    }
    builder.result
  }

  private[kernel] def fileBasedState = FileBasedState.values()(cppProxy.file_based_state)

  private def fileBasedErrorMessageOption: Option[String] =
    emptyToNone(cppProxy.file_based_error_string)

  protected def pathOrKey: ThisPath = path

  private[kernel] final def path: ThisPath =
    _fixedPath getOrElse {
      if (cppProxy.name_is_fixed) {
        _fixedPath.trySet(stringToPath(cppProxy.path))
        _fixedPath()
      } else
        stringToPath(cppProxy.path)
    }

  private[kernel] def name = cppProxy.name

  private def fileModificationInstantOption: Option[Instant] =
    cppProxy.file_modification_time_t match {
      case 0 ⇒ None
      case n ⇒ Some(Instant.ofEpochSecond(n))
    }

  private def sourceXmlBytes: Array[Byte] = cppProxy.source_xml_bytes

  def configurationXmlBytes = inSchedulerThread { cppProxy.source_xml_bytes }

  def file: Path = inSchedulerThread { fileOption } getOrElse sys.error(s"$toString has no source file")

  private def fileOption: Option[Path] =
    cppProxy.file match {
      case "" ⇒ None
      case o ⇒ Some(Paths.get(o))
    }

  /** Markiert, dass das [[com.sos.scheduler.engine.kernel.filebased.FileBased]] beim nächsten Verzeichnisabgleich neu geladen werden soll. */
  private[kernel] def forceFileReread(): Unit = {
    cppProxy.set_force_file_reread()
  }

  /** @return true, wenn das [[com.sos.scheduler.engine.kernel.filebased.FileBased]] nach einer Änderung erneut geladen worden ist. */
  def fileBasedIsReread =
    inSchedulerThread {
      cppProxy.is_file_based_reread
    }

  def isVisible: Boolean = inSchedulerThread { cppProxy.is_visible }

  def isFileBased: Boolean = inSchedulerThread { cppProxy.is_file_based }

  protected def hasBaseFile: Boolean = _fixedPath.isDefined || cppProxy.has_base_file

  def log: PrefixLog = inSchedulerThread { cppProxy.log.getSister }

  override def toString = getClass.getName + (_fixedPath.toOption map { o ⇒ s"('$o')" } getOrElse "")

  def stringToPath(o: String): ThisPath

  private[kernel] final def configurationFileRemoved = cppProxy.is_to_be_removed

  private[kernel] final def replacementOption: Option[Self] = Option(cppProxy.replacement_java.asInstanceOf[Self])

  private[kernel] final def missingRequisites: Set[TypedPath] =
    (cppProxy.missing_requisites_java map TypedPathRegister.fromCppTypedString).toSet

  protected def fixedPathOption = _fixedPath.toOption
}
