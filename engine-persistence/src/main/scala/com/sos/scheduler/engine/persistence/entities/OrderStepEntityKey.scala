package com.sos.scheduler.engine.persistence.entities

/**
 * @author Joacim Zschimmer
 */
case class OrderStepEntityKey(orderHistoryId: Int, step: Int) extends Serializable {
  def this() = this(0, 0)
}
