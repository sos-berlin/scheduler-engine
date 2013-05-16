package com.sos.scheduler.engine.newkernel.job

import com.sos.scheduler.engine.common.async.CallQueue
import com.sos.scheduler.engine.data.job.TaskId

final class ShellTask(val id: TaskId, script: ShellScript, job: NewJob, callQueue: CallQueue)
extends ProcessTerminatedHandler {

  private val process = new ShellProcess(callQueue, this)

  def start() {
    process.start(script)
  }

  def kill() {
    process.kill()
  }

  def end() {
    ???
  }

  def onProcessTerminated() {
    job.onTaskTerminated(this)
  }
}
