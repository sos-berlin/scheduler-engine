package com.sos.scheduler.engine.kernel.job

import com.sos.jobscheduler.data.job.TaskId

final class TaskNotFoundException(taskId: TaskId)
extends RuntimeException(s"Task ${taskId.string} is unknown")
