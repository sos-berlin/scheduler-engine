package com.sos.scheduler.engine.persistence.entities

import java.io.Serializable

case class OrderEntityKey(var schedulerId: String, var jobChainPath: String, var orderId: String) extends Serializable {
  def this() = this(null: String, null: String, null: String)
}
