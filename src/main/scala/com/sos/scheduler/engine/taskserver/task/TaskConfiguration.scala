package com.sos.scheduler.engine.taskserver.task

import com.sos.scheduler.engine.data.job.TaskId

/**
 * @author Joacim Zschimmer
 */
final case class TaskConfiguration(
  jobName: String,
  taskId: TaskId,
  language: ScriptLanguage,
  script: String) {
}
