package com.sos.scheduler.engine.kernel.job

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.eventbus.EventSource
import com.sos.scheduler.engine.kernel.order.UnmodifiableOrder

@ForCpp
trait UnmodifiableTask extends EventSource {
  def id: TaskId
  def job: UnmodifiableJob
  def orderOption: Option[UnmodifiableOrder]
  def parameterValue(name: String): String
}
