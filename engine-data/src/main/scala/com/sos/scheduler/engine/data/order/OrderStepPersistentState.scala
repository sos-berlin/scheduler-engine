package com.sos.scheduler.engine.data.order

import com.sos.scheduler.engine.data.base.HasKey
import com.sos.scheduler.engine.data.job.TaskId
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order.OrderStepPersistentState.Key
import java.time.Instant

/**
 * @author Joacim Zschimmer
 */
final case class OrderStepPersistentState (
  orderHistoryId: OrderHistoryId,
  step: Int,
  taskId: TaskId,
  orderState: OrderState,
  startTime: Instant,
  endTimeOption: Option[Instant],
  isError: Boolean,
  errorCodeOption: Option[MessageCode],
  errorText: String)
extends HasKey[Key] {
  def key = Key(orderHistoryId, step)
}

object OrderStepPersistentState {
  final case class Key(orderHistoryId: OrderHistoryId, step: Int)
}
