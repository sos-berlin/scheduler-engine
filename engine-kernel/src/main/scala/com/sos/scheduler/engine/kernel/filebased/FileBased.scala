package com.sos.scheduler.engine.kernel.filebased

import com.sos.scheduler.engine.common.xml.XmlUtils.xmlBytesToString
import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.filebased._
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
  self ⇒

  type ThisPath <: TypedPath

  protected def subsystem: FileBasedSubsystem

  protected implicit def schedulerThreadCallQueue: SchedulerThreadCallQueue = subsystem.schedulerThreadCallQueue

  /** Jedes Exemplar hat seine eigene UUID. */
  final val uuid = java.util.UUID.randomUUID

  protected[this] def cppProxy: File_basedC[_]

  def overview: FileBasedOverview =
    inSchedulerThread {
      SimpleFileBasedOverview(
        path = self.path,
        fileBasedState = self.fileBasedState)
    }

  def details: FileBasedDetails =
    inSchedulerThread {
      val overview = self.overview
      SimpleFileBasedDetails(
        path = overview.path,
        fileBasedState = overview.fileBasedState,
        file = fileOption,
        fileModifiedAt = fileModificationInstantOption,
        sourceXml = sourceXmlBytes match {
          case o if o.isEmpty ⇒ None
          case o ⇒
            try Some(xmlBytesToString(o))
            catch { case NonFatal(t) ⇒ Some(<ERROR>{t.toString}</ERROR>.toString()) }
        })
    }

  def fileBasedType: FileBasedType

  def fileBasedState =
    FileBasedState.values()(cppProxy.file_based_state)

  /** Für Java. */
  def getPath: TypedPath = path

  def path: ThisPath = stringToPath(cppProxy.path)

  def name =
    cppProxy.name

  def fileModificationInstantOption: Option[Instant] =
    cppProxy.file_modification_time_t match {
      case 0 ⇒ None
      case n ⇒ Some(Instant.ofEpochSecond(n))
    }

  def sourceXmlBytes: Array[Byte] =
    cppProxy.source_xml_bytes

  def configurationXmlBytes =
    cppProxy.source_xml_bytes

  def file: Path = fileOption getOrElse sys.error(s"$toString has no source file")

  def fileOption: Option[Path] =
    cppProxy.file match {
      case "" ⇒ None
      case o ⇒ Some(Paths.get(o))
    }

  /** Markiert, dass das [[com.sos.scheduler.engine.kernel.filebased.FileBased]] beim nächsten Verzeichnisabgleich neu geladen werden soll. */
  def forceFileReread(): Unit = {
    cppProxy.set_force_file_reread()
  }

  /** @return true, wenn das [[com.sos.scheduler.engine.kernel.filebased.FileBased]] nach einer Änderung erneut geladen worden ist. */
  def fileBasedIsReread =
    cppProxy.is_file_based_reread

  def isVisible: Boolean = cppProxy.is_visible
  def hasBaseFile: Boolean = cppProxy.has_base_file
  def isToBeRemoved: Boolean = cppProxy.is_to_be_removed

  def log: PrefixLog =
    cppProxy.log.getSister

  override def toString = path.toString

  def stringToPath(o: String): ThisPath
}
