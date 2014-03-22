package com.sos.scheduler.engine.kernel.filebased

import com.sos.scheduler.engine.cplusplus.runtime.Sister
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.filebased.{FileBasedType, TypedPath}
import com.sos.scheduler.engine.eventbus.EventSource
import com.sos.scheduler.engine.kernel.cppproxy.File_basedC
import com.sos.scheduler.engine.kernel.log.PrefixLog
import java.io.File

@ForCpp
abstract class FileBased
extends Sister
with EventSource {

  type Path <: TypedPath

  /** Jedes Exemplar hat seine eigene UUID. */
  final val uuid = java.util.UUID.randomUUID

  protected[this] def cppProxy: File_basedC[_]

  def fileBasedType: FileBasedType

  def fileBasedState =
    FileBasedState.ofCppName(cppProxy.file_based_state_name)

  def path: Path =
    stringToPath(cppProxy.path)

  def name =
    cppProxy.name

  def configurationXmlBytes =
    cppProxy.source_xml_bytes

  def file: File =
    cppProxy.file match {
      case "" => sys.error(s"$toString has no source file")
      case o => new File(o)
    }

  /** Markiert, dass das [[com.sos.scheduler.engine.kernel.filebased.FileBased]] beim nächsten Verzeichnisabgleich neu geladen werden soll. */
  def forceFileReread() {
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

  def stringToPath(o: String): Path
}
