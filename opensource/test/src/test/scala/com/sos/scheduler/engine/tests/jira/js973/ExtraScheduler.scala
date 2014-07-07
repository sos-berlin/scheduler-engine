package com.sos.scheduler.engine.tests.jira.js973

import ExtraScheduler._
import com.google.common.base.Strings.nullToEmpty
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.system.OperatingSystem
import com.sos.scheduler.engine.common.system.OperatingSystem.operatingSystem
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants._
import java.io.{File, InputStream, InputStreamReader}
import java.nio.charset.Charset
import scala.collection.JavaConversions._
import scala.collection.immutable
import scala.concurrent.promise

final class ExtraScheduler(args: immutable.Seq[String], env: Iterable[(String, String)], tcpPort: Int)
    extends AutoCloseable {

  private var process: Process = null
  private val tcpIsReadyPromise = promise[Boolean]()

  def tcpIsReadyFuture =
    tcpIsReadyPromise.future

  private val stdoutThread = new Thread("stdout collector") {
    override def run() {
      try
        unbufferedInputStreamToLines(process.getInputStream, schedulerEncoding) { line =>
          logger info line
          if ((line contains " SCHEDULER-956 ") && (line contains " TCP"))
            tcpIsReadyPromise success true
        }
      catch {
        case t: Throwable =>
          logger.error(s"Thread aborts with $t", t)
          throw t
      }
      finally
        tcpIsReadyPromise tryFailure new RuntimeException("ExtraScheduler has not started successfully")
    }
  }

  def start() {
    if (process != null) throw new IllegalStateException()
    val processBuilder = new ProcessBuilder(args :+ s"-tcp-port=$tcpPort")
    if (OperatingSystem.isUnix) {
      val name = operatingSystem.getDynamicLibraryEnvironmentVariableName
      val previous = nullToEmpty(System.getenv(name))
      processBuilder.environment().put(name, OperatingSystem.concatFileAndPathChain(new File(args.head).getParentFile, previous))
    }
    for ((k, v) <- env) processBuilder.environment().put(k, v)
    processBuilder.redirectErrorStream(true)
    process = processBuilder.start()
    stdoutThread.start()
  }

  def close() {
    if (process != null) {
      process.destroy()
      process.waitFor()
      stdoutThread.interrupt()
      stdoutThread.join()
    }
  }

  def address =
    SchedulerAddress(s"127.0.0.1:$tcpPort")

  override def toString =
    s"ExtraScheduler($address)"
}

private object ExtraScheduler {
  private def logger = Logger(getClass)

  private def unbufferedInputStreamToLines(in: InputStream, encoding: Charset)(processLine: String => Unit) {
    val lineBuffer = new StringBuffer(1000)
    val i = new ReaderIterator(new InputStreamReader(in, encoding))
    while (i.hasNext) {
      i takeWhile { _ != '\n' } foreach lineBuffer.append
      val line = lineBuffer.toString stripSuffix "\r"
      processLine(line)
      lineBuffer.delete(0, lineBuffer.length)
    }
  }
}
