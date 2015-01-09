package com.sos.scheduler.engine.taskserver.task.process

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.job.ResultCode
import com.sos.scheduler.engine.taskserver.task.process.ShellProcess._
import com.sos.scheduler.engine.taskserver.task.process.StdoutStderr.StdoutStderrType
import java.nio.charset.Charset
import java.nio.file.Path
import java.util.concurrent.TimeUnit
import scala.collection.immutable
import scala.concurrent.Promise

/**
 * @author Joacim Zschimmer
 */
final class ShellProcess private[process](process: Process, file: Path, stdFiles: Map[StdoutStderrType, Path], fileEncoding: Charset) extends HasCloser {

  private val promise = Promise[Unit]()
  def closedFuture = promise.future

  override def close() {
    try super.close()
    finally promise.success(())
  }

  def waitForTermination(logOutput: String ⇒ Unit) = {
    autoClosing(new FilesLineCollector(Nil ++ stdFiles.values, fileEncoding)) { fileLogger ⇒
      while (!process.waitFor(LoggingPausePeriod.getMillis, TimeUnit.MILLISECONDS)) {     // Die waitFor-Implementierung fragt millisekündlich ab
        fileLogger.nextLinesIterator foreach logOutput
      }
      fileLogger.nextLinesIterator foreach logOutput
    }
    ResultCode(process.exitValue)
  }

  override def toString = s"$process $file"

  def files: immutable.Seq[Path] = List(file) ++ stdFiles.values
}

object ShellProcess {
  private val LoggingPausePeriod = 1.s
}
