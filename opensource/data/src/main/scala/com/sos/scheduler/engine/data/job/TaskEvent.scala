package com.sos.scheduler.engine.data.job

import com.sos.scheduler.engine.data.event.{KeyedEvent, Event}
import com.sos.scheduler.engine.data.folder.JobPath

trait TaskEvent extends KeyedEvent {
  type Key = TaskId
  final def key = taskId

  def taskId: TaskId
  def jobPath: JobPath
}
