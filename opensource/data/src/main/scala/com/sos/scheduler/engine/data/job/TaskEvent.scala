package com.sos.scheduler.engine.data.job

import com.sos.scheduler.engine.data.event.Event
import com.sos.scheduler.engine.data.folder.JobPath

trait TaskEvent extends Event {
  def taskId: TaskId
  def jobPath: JobPath
}
