package com.sos.scheduler.engine.data.jobchain

import com.sos.scheduler.engine.data.order.OrderState

final case class EndNodeOverview(orderState: OrderState)
extends NodeOverview
