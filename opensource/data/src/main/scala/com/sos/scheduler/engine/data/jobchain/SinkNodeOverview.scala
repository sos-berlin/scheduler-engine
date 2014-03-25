package com.sos.scheduler.engine.data.jobchain

import com.sos.scheduler.engine.data.order.OrderState

final case class SinkNodeOverview(orderState: OrderState)
extends NodeOverview
