package com.sos.scheduler.engine.taskserver.task

import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.HasCloser
import com.sos.scheduler.engine.taskserver.task.process.{ShellProcess, ShellProcessStarter}

/**
 * @author Joacim Zschimmer
 */
final class ShellProcessTask(conf: TaskConfiguration) extends Task with HasCloser {

  private var process: ShellProcess = null

  override def start() = {
    process = ShellProcessStarter.start(
      name = conf.jobName,
      scriptString = conf.scriptString)
    closer.registerAutoCloseable(process)
  }

  override def end() = {}

  override def step() = {
    val exitValue = process.waitFor()
    <process.result spooler_process_result="true" exit_code={exitValue.toString}/>.toString()
  }
}
