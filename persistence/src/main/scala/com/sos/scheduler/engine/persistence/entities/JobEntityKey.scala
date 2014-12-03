package com.sos.scheduler.engine.persistence.entities

case class JobEntityKey(var schedulerId: String, var clusterMemberId: String, var jobPath: String) extends Serializable {
  def this() = this(null: String, null: String, null: String)
}
