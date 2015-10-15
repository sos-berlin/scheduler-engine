package com.sos.scheduler.engine.data.order

import com.sos.scheduler.engine.data.base.HasKey
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.scheduler.SchedulerId
import java.time.Instant

/**
 * @author Joacim Zschimmer
 */
final case class OrderHistoryPersistentState (
  historyId: OrderHistoryId,
  jobChainPath: JobChainPath,
  orderId: OrderId,
  schedulerId: SchedulerId,
  title: String,
  orderState: OrderState,
  stateText: String,
  startTime: Instant,
  endTimeOption: Option[Instant],
  log: String)
extends HasKey[OrderHistoryId] {
  def key = historyId
}
