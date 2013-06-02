package com.sos.scheduler.engine.data.base

import com.fasterxml.jackson.databind.annotation.JsonSerialize

@JsonSerialize(using = classOf[GenericIntSerializer])
trait GenericInt {
  def value: Int
}
