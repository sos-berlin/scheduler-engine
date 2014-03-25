package com.sos.scheduler.engine.data.jobchain

import com.sos.scheduler.engine.data.order.OrderState

final case class NestedJobChainNodeOverview(
  orderState: OrderState,
  nextState: OrderState,
  errorState: OrderState,
  nestedJobChainPath: JobChainPath)
extends NodeOverview
