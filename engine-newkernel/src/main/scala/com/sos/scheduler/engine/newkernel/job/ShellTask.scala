package com.sos.scheduler.engine.newkernel.job

import com.sos.jobscheduler.data.job.TaskId
import com.sos.scheduler.engine.common.async.CallQueue

final class ShellTask(val id: TaskId, script: ShellScript, job: NewJob, callQueue: CallQueue)
extends ProcessTerminatedHandler {

  private val process = new ShellProcess(callQueue, this)

  def start(): Unit = {
    process.start(script)
  }

  def kill(): Unit = {
    process.kill()
  }

  def end(): Unit = {
    ???
  }

  def onProcessTerminated(): Unit = {
    job.onTaskTerminated(this)
  }
}
