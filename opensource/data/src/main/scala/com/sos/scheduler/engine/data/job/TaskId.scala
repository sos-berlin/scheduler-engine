package com.sos.scheduler.engine.data.job

import com.fasterxml.jackson.annotation.{JsonIgnore, JsonCreator, JsonValue}
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import scala.annotation.meta.getter
import com.sos.scheduler.engine.data.base.GenericInt

@ForCpp
final case class TaskId(@(ForCpp @getter) value: Int) extends GenericInt {

  def string = value.toString
}

object TaskId {
  @JsonCreator def jsonCreator(taskId: Int) =
    new TaskId(taskId)
}
