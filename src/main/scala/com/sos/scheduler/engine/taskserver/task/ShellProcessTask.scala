package com.sos.scheduler.engine.taskserver.task

import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.taskserver.task.process.{ShellProcess, ShellProcessStarter}

/**
 * @author Joacim Zschimmer
 */
final class ShellProcessTask(conf: TaskConfiguration, log: String ⇒ Unit) extends Task with HasCloser {

  private var process: ShellProcess = null

  override def start() = {
    process = ShellProcessStarter.start(
      name = conf.jobName,
      extraEnvironment = conf.extraEnvironment,
      scriptString = conf.script)
    closer.registerAutoCloseable(process)
  }

  override def end() = {}

  override def step() = {
    val resultCode = process.waitForTermination(logOutputLine = log)
    <process.result
      state_text={process.firstStdoutLine}
      spooler_process_result="true"
      exit_code={resultCode.value.toString}/>.toString()
  }

  def files = {
    if (process == null) throw new IllegalStateException
    process.files
  }
}
