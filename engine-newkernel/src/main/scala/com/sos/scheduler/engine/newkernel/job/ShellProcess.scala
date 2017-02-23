package com.sos.scheduler.engine.newkernel.job

import com.google.common.base.Preconditions.checkState
import com.google.common.io.Files
import com.sos.jobscheduler.common.scalautil.Logger
import com.sos.jobscheduler.common.system.OperatingSystem.isWindows
import com.sos.scheduler.engine.common.async.CallQueue
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.newkernel.job.ShellProcess._
import com.sos.scheduler.engine.newkernel.utils.{ThreadService, TimedCallHolder}
import java.io.File
import java.nio.charset.Charset
import java.nio.charset.Charset.defaultCharset
import org.joda.time.Instant
import scala.concurrent.blocking

final class ShellProcess(
    callQueue: CallQueue,
    processTerminatedHandler: ProcessTerminatedHandler)
extends AutoCloseable {

  private var starter: Starter = null
  private var process: Process = null
  private val processTerminatedCheckerCallHolder = new TimedCallHolder(callQueue)
  private var stdLoggers = Seq[ThreadService]()

  def start(script: ShellScript): Unit = {
    startProcess(script)
    startProcessTerminatedChecker()
  }

  private def startProcess(shellScript: ShellScript): Unit = {
    checkState(process == null)
    starter = if (isWindows) new WindowsStarter else new UnixStarter
    val processBuilder = starter.newProcessBuilder(shellScript)
    logger debug s"Start shell script $shellScript"
    process = processBuilder.start()

    stdLoggers = Seq(process.getInputStream, process.getErrorStream) map {
      o => new ThreadService(new InputStreamLogger(o, Charset.defaultCharset, logger.underlying))
    }
    for (o <- stdLoggers) o.start()
  }

  private def startProcessTerminatedChecker(): Unit = {
    processTerminatedCheckerCallHolder.enqueue(Instant.now() + 100.ms, "ShellProcess.isTerminated") {
      if (isTerminated) {
        logger debug "Process terminated"
        processTerminatedHandler.onProcessTerminated()
      } else
        startProcessTerminatedChecker()
    }
  }

  def isTerminated =
    try {
      process.exitValue
      true
    }
    catch { case _: IllegalThreadStateException => false }

  def kill(): Unit = {
    process.destroy()
  }

  def close(): Unit = {
    try
      if (process != null) {
        process.destroy()
        process.waitFor()
        for (o ← stdLoggers) {
          blocking {
            o.thread.join() // Sollte asynchron sein
          }
          o.close()
        }
    }
    finally
      if (starter != null) starter.close()
  }
}

object ShellProcess {
  private val logger = Logger(getClass)

  trait Starter {
    def file: File
    def newProcessBuilder(shellScript: ShellScript): ProcessBuilder

    def close(): Unit = {
      if (file != null)
        file.delete()   // Verzögert löschen? (s_deleting_files)
    }
  }

  final class UnixStarter extends Starter {
    var file: File = null

    def newProcessBuilder(shellScript: ShellScript) = {
      file = File.createTempFile("sos-", ".sh")
      Files.write(shellScript.text, file, defaultCharset)
      val processBuilder = new ProcessBuilder
      processBuilder.command("/bin/sh", "-c", shellScript.toString)
      processBuilder
    }
  }

  final class WindowsStarter extends Starter {
    var file: File = null

    def newProcessBuilder(shellScript: ShellScript) = {
      file = File.createTempFile("sos", ".cmd")
      Files.write(shellScript.text, file, defaultCharset)
      val processBuilder = new ProcessBuilder
      processBuilder.command("""c:\windows\system32\cmd.exe""", "/C", shellScript.toString)
      processBuilder
    }
  }
}
