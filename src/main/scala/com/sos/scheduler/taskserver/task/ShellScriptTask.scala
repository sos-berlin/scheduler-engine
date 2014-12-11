package com.sos.scheduler.taskserver.task

import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.taskserver.comrpc.IUnknown
import java.nio.charset.StandardCharsets.ISO_8859_1
import java.nio.file.Files

/**
 * @author Joacim Zschimmer
 */
final class ShellScriptTask(objectMap: Map[String, IUnknown], script: String) extends Task {

  private val scriptFile = Files.createTempFile("JobScheduler", ".cmd").toFile

  override def start() = {
    scriptFile.write(script, ISO_8859_1)
    true
  }

  override def end() = {}

  override def step() = {
    val b = new ProcessBuilder
    b.command(s"cmd", "/c", scriptFile.toString)
    val process = b.start()  // Used native function is synchronized (one after another)
    val exitValue = process.waitFor()
    Files.delete(scriptFile)
    <process.result spooler_process_result="true" exit_code={exitValue.toString} state_text="STATE_TEXT"/>.toString()
  }
}

private object ShellScriptTask {
  private val logger = Logger(getClass)
}
