package com.sos.scheduler.engine.data.job

import com.sos.scheduler.engine.data.job.JobPath

final case class TaskStartedEvent(taskId: TaskId, jobPath: JobPath)
extends TaskEvent
