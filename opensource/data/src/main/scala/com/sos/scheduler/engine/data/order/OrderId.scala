package com.sos.scheduler.engine.data.order

import com.sos.scheduler.engine.data.base.IsString
import com.fasterxml.jackson.annotation.JsonCreator

final case class OrderId(string: String)
extends IsString {

  @Deprecated def asString = string
}


object OrderId {
  @JsonCreator def valueOf(string: String): OrderId =
    new OrderId(string)
}