package com.sos.scheduler.engine.newkernel.job

import ShellProcess._
import com.google.common.base.Preconditions.checkState
import com.google.common.io.Files
import com.sos.scheduler.engine.common.async.CallQueue
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.utils.SosAutoCloseable
import com.sos.scheduler.engine.newkernel.utils.{Service, ThreadService, TimedCallHolder}
import java.io.File
import java.nio.charset.Charset
import java.nio.charset.Charset.defaultCharset
import org.joda.time.Instant

final class ShellProcess(
    callQueue: CallQueue,
    processTerminatedHandler: ProcessTerminatedHandler)
extends SosAutoCloseable {

  private var file: File = null
  private var process: Process = null
  private val processTerminatedCheckerCallHolder = new TimedCallHolder(callQueue)
  private var stdLoggers = Seq[ThreadService]()

  def start(script: ShellScript) {
    startProcess(script)
    startProcessTerminatedChecker()
  }

  private def startProcess(script: ShellScript) {
    checkState(process == null)
    file = File.createTempFile("sos", ".cmd")
    Files.write(script.text, file, defaultCharset)
    val processBuilder = new ProcessBuilder
    logger debug s"Start script $file"
    processBuilder.command("c:\\windows\\system32\\cmd.exe", "/C", file.toString)
    process = processBuilder.start()
    stdLoggers = Seq(process.getInputStream, process.getErrorStream) map {
      o => new ThreadService(new InputStreamLogger(o, Charset.defaultCharset, logger.delegate))
    }
    for (o <- stdLoggers) o.start()
  }

  private def startProcessTerminatedChecker() {
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

  def kill() {
    process.destroy()
  }

  def close() {
    try if (process != null) {
      process.destroy()
      process.waitFor()
      for (o <- stdLoggers) {
        o.thread.join() // Sollte asynchron sein
        o.close()
      }
    }
    finally if (file != null)
      file.delete()   // Verzögert löschen? (s_deleting_files)
  }
}

object ShellProcess {
  private val logger = Logger[ShellProcess]
}