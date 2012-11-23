package com.sos.scheduler.engine.data.job

import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import org.codehaus.jackson.annotate.{JsonCreator, JsonValue}
import scala.annotation.target.getter

@ForCpp case class TaskId(
  @(JsonValue @getter) @(ForCpp @getter) value: Int)

object TaskId {
  @JsonCreator def jsonCreator(taskId: Int) = new TaskId(taskId)
}
