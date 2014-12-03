package com.sos.scheduler.engine.newkernel.utils

object StringConverters {
  def stringToBoolean(o: String) = o match {
    case "true" | "yes" | "1" => true
    case "false" | "no" | "0" => false
  }
}
