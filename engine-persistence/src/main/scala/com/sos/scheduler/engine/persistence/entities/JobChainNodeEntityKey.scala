package com.sos.scheduler.engine.persistence.entities

import java.io.Serializable

case class JobChainNodeEntityKey(var schedulerId: String, var clusterMemberId: String, var jobChainPath: String, orderState: String) extends Serializable {
  def this() = this(null: String, null: String, null: String, null: String)
}
