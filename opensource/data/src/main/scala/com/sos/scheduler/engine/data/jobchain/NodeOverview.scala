package com.sos.scheduler.engine.data.jobchain

import com.sos.scheduler.engine.data.order.OrderState

trait NodeOverview {
  val orderState: OrderState
}
