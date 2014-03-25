package com.sos.scheduler.engine.kernel.order.jobchain

import com.sos.scheduler.engine.data.order.OrderState

trait NodeOverview {
  val orderState: OrderState
}
