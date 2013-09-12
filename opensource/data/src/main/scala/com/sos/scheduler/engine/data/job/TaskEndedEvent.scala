package com.sos.scheduler.engine.data.job

import com.sos.scheduler.engine.data.folder.JobPath

final case class TaskEndedEvent(taskId: TaskId, jobPath: JobPath)
extends TaskEvent
