package com.sos.scheduler.engine.persistence.entities

import scala.beans.BeanProperty
import java.io.Serializable

case class JobChainEntityKey (
                  @BeanProperty var schedulerId: String,
                  @BeanProperty var clusterMemberId: String,
                  @BeanProperty var jobChainPath: String)
    extends Serializable {
  def this() = this(null: String, null: String, null: String)
}
