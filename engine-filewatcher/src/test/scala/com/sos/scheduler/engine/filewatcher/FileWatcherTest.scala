package com.sos.scheduler.engine.filewatcher

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.filewatcher.FileWatcher._
import java.nio.file.Files
import java.nio.file.Files.createFile
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.JavaConversions._

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class FileWatcherTest extends FreeSpec with HasCloser {

  private lazy val directory = Files.createTempDirectory("test-")

  onClose {
    directory.toList foreach Files.delete
    Files.delete(directory)
  }

  "File changes" in {
    val fileWatcher = new FileWatcher(directory)
    assert(fileWatcher.nextChanges().isEmpty)
    val a = createFile(directory / "a")
    fileWatcher.nextChanges() shouldEqual List(FileAdded(a))
    val b = createFile(directory / "b")
    fileWatcher.nextChanges() shouldEqual List(FileAdded(b))
    Files.delete(b)
    val c = createFile(directory / "c")
    val d = createFile(directory / "d")
    fileWatcher.nextChanges().toSet shouldEqual Set(FileRemoved(b), FileAdded(c), FileAdded(d))
  }
}

/*
  XML-Kommando, das COM-Server mit neuer COM-Klasse AgentFileOrderSource einrichtet.
  XML-Kommando, das COM-Server schließt. Dabei wird laufender Aufruf awaitDirectoryChange() beendet.
  COM-Klasse AgentFileOrderSource(dir, regex, timeout)
  mit Method awaitDirectoryChange,  ** Wie gleichen wir mit den Aufträgen ab?
  wartet Änderung ab,
  liefert die neuen und gelöschten Dateinamen.
*/
