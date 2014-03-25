package com.sos.scheduler.engine.kernel.order.jobchain

import com.sos.scheduler.engine.data.order.OrderState

final case class SinkNodeOverview(orderState: OrderState)
extends NodeOverview
