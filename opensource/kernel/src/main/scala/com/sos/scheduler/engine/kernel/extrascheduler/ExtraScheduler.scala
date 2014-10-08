package com.sos.scheduler.engine.kernel.extrascheduler

import com.google.common.base.Strings._
import com.sos.scheduler.engine.common.io.ReaderIterator
import com.sos.scheduler.engine.common.scalautil.{HasCloser, Logger}
import com.sos.scheduler.engine.common.system.OperatingSystem
import com.sos.scheduler.engine.common.system.OperatingSystem._
import com.sos.scheduler.engine.kernel.extrascheduler.ExtraScheduler._
import com.sos.scheduler.engine.kernel.extrascheduler.ExtraScheduler.logger
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants._
import java.io.{File, IOException, InputStream, InputStreamReader}
import java.net.URI
import java.nio.charset.Charset
import scala.collection.JavaConversions._
import scala.collection.immutable
import scala.concurrent.Promise

final class ExtraScheduler(
  args: immutable.Seq[String],
  env: immutable.Iterable[(String, String)],
  httpPort: Option[Int] = None,
  tcpPort: Option[Int] = None)
extends AutoCloseable with HasCloser {

  private val tcpAddressOption = tcpPort map { o ⇒ SchedulerAddress("127.0.0.1", o) }
  private val uriOption = httpPort map { o ⇒ new URI(s"http://127.0.0.1:$o") }
  val name = (uriOption orElse tcpAddressOption getOrElse sys.error("httpPort and tcpPort not given")).toString
  private val isActivePromise = Promise[Unit]()

  onClose { isActivePromise.tryFailure(new IllegalStateException("ExtraScheduler has been closed") ) }

  def start(): Unit = {
    closeOnError {
      val process = startProcess()
      whenNotClosedAtShutdown {
        process.destroy()
        logger.error(s"ExtraScheduler.close() has not been called. Process is being destroyed")
      }
      startStdoutCollectorThread(process.getInputStream)
      onClose {
        process.destroy()  // StdoutCollectorThread endet erst, wenn Prozess geendet hat
        process.waitFor()
      }
    }
  }

  private def startProcess(): Process = {
    val httpArgs = httpPort map { o ⇒ s"-http-port=$o" }
    val tcpArgs = tcpAddressOption.toList flatMap { case SchedulerAddress(interface, port) ⇒ List(s"-tcp-port=$port", s"-ip-address=$interface") }
    val processBuilder = new ProcessBuilder(args ++ httpArgs ++ tcpArgs)
    if (OperatingSystem.isUnix) {
      val name = operatingSystem.getDynamicLibraryEnvironmentVariableName
      val previous = nullToEmpty(System.getenv(name))
      processBuilder.environment().put(name, OperatingSystem.concatFileAndPathChain(new File(args.head).getParentFile, previous))
    }
    for ((k, v) <- env) processBuilder.environment().put(k, v)
    processBuilder.redirectErrorStream(true)
    processBuilder.start()
  }

  private def startStdoutCollectorThread(in: InputStream): Unit = {
    new Thread(s"Stdout collector $name") {
      override def run(): Unit = {
        val expectedMessageCode = "SCHEDULER-902"
        try
          unbufferedInputStreamToLines(in, schedulerEncoding) { line ⇒
            logger.info(line)
            if (!isActivePromise.isCompleted && (line contains s" $expectedMessageCode ") && (line contains " state=running"))
              isActivePromise.success(())
          }
        catch {
          case t: Throwable ⇒
            logger.error(s"Thread aborts with $t", t)
            throw t
        }
        finally
          isActivePromise.tryFailure(new RuntimeException(s"ExtraScheduler has not started successfully with message $expectedMessageCode"))
      }
      start()
      onClose {
        interrupt()
        join()
      }
    }
  }

  override def close(): Unit = {
    logger.debug(s"Start closing $this")
    super.close()
    logger.debug(s"Finished closing $this")
  }

  def isActiveFuture = isActivePromise.future

  def uri: URI = uriOption getOrElse sys.error("httpPort not given")

  def tcpAddress: SchedulerAddress = tcpAddressOption getOrElse sys.error("httpPort not given")

  override def toString = s"ExtraScheduler($name)"
}

private object ExtraScheduler {
  private val logger = Logger(getClass)

  private def unbufferedInputStreamToLines(in: InputStream, encoding: Charset)(processLine: String ⇒ Unit): Unit = {
    val lineBuffer = new StringBuilder(1000)
    val i = new ReaderIterator(new InputStreamReader(in, encoding))
    try {
      while (i.hasNext) {
        i takeWhile { _ != '\n' } foreach lineBuffer.append
        val line = lineBuffer.toString stripSuffix "\r"
        processLine(line)
        lineBuffer.delete(0, lineBuffer.length)
      }
    } catch {
      case e: IOException ⇒  // "Stream closed" under Linux
    }
  }
}
