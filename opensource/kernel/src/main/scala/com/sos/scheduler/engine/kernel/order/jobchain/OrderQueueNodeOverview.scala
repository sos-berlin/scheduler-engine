package com.sos.scheduler.engine.kernel.order.jobchain

import com.sos.scheduler.engine.data.jobchain.JobChainNodeAction
import com.sos.scheduler.engine.data.order.OrderState

trait OrderQueueNodeOverview extends NodeOverview {
  def nextState: OrderState
  def errorState: OrderState
  def action: JobChainNodeAction
  def orderCount: Int
}
