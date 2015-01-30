package com.sos.scheduler.engine.filewatcher

import com.sos.scheduler.engine.common.scalautil.Collections.implicits._
import com.sos.scheduler.engine.filewatcher.FileWatcher._
import java.nio.file.{Files, Path}
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
final class FileWatcher(directory: Path) extends Mutable {

  private var lastContent = Set[Path]()

  def nextChanges(): immutable.Seq[FileChange] = {
    val content = listFiles(directory)
    val added = (content -- lastContent).toVector map FileAdded
    val removed = (lastContent -- content).toVector map FileRemoved
    lastContent = content
    removed ++ added
  }
}

object FileWatcher {
  private def listFiles(dir: Path) = Files.list(dir).toSet

  trait FileChange {
    def file: Path
  }

  final case class FileAdded(file: Path) extends FileChange

  final case class FileRemoved(file: Path) extends FileChange
}
