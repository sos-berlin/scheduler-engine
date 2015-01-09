package com.sos.scheduler.engine.taskserver.task.process

import com.google.common.collect.{AbstractIterator ⇒ GuavaAbstractIterator}
import com.sos.scheduler.engine.common.scalautil.AutoClosing.closeOnError
import com.sos.scheduler.engine.common.scalautil.Closers.implicits.RichClosersAutoCloseable
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.taskserver.task.process.FilesLineCollector._
import java.io.{BufferedReader, FileInputStream, InputStream, InputStreamReader}
import java.nio.charset.Charset
import java.nio.file.Path
import scala.collection.JavaConversions._
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
final class FilesLineCollector(files: immutable.Iterable[Path], encoding: Charset) extends HasCloser {

  private val readers = closeOnError(closer) { files map { f ⇒ new FileLineCollector(f, encoding).closeWithCloser } }

  def nextLinesIterator: Iterator[String] = readers.iterator flatMap { _.nextLinesIterator }
}

private object FilesLineCollector {
  private class FileLineCollector(file: Path, encoding: Charset) extends HasCloser {
    private val collector = new LineCollector(new FileInputStream(file).closeWithCloser, encoding)
    def nextLinesIterator: Iterator[String] = collector.nextLinesIterator()
  }

  private class LineCollector(in: InputStream, encoding: Charset) {
    private val reader = new BufferedReader(new InputStreamReader(in, encoding))

    def nextLinesIterator() = asScalaIterator(new GuavaAbstractIterator[String] {
      def computeNext() = reader.readLine() match {
        case null ⇒ endOfData()
        case o ⇒ o
      }
    })
  }
}
