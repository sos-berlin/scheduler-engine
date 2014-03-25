package com.sos.scheduler.engine.kernel.order.jobchain

import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.jobchain.JobChainNodeAction
import com.sos.scheduler.engine.data.order.OrderState

final case class JobNodeOverview(
  orderState: OrderState,
  nextState: OrderState,
  errorState: OrderState,
  action: JobChainNodeAction,
  jobPath: JobPath,
  orderCount: Int)
extends OrderQueueNodeOverview
