package com.sos.scheduler.engine.data.job

import com.fasterxml.jackson.annotation.{JsonCreator, JsonValue}
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import scala.annotation.meta.getter

@ForCpp
final case class TaskId(
  @(JsonValue @getter) @(ForCpp @getter) value: Int) {
}

object TaskId {
  @JsonCreator def jsonCreator(taskId: Int) =
    new TaskId(taskId)
}
