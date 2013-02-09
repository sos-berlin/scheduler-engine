package com.sos.scheduler.engine.playground.plugins.jobnet.parallel

import com.sos.scheduler.engine.data.base.IsString
import com.sos.scheduler.engine.data.order.OrderId

final case class BranchId(string: String) extends IsString {
  def branchOrderId(o: OrderId) = BranchOrderId(this, o)
}

