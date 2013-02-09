package com.sos.scheduler.engine.playground.plugins.jobnet.parallel

import com.sos.scheduler.engine.data.order.OrderId

object BranchOrderId {
  val Pattern = """^(.*)/([^/]+)$"""r

  def apply(branchId: BranchId, o: OrderId) = new OrderId(o.string +"/"+ branchId.string)

  def unapply(o: OrderId): Option[(BranchId, OrderId)] =
    Pattern.findFirstMatchIn(o.string) map { m => (BranchId(m.group(0)), new OrderId(m.group(1))) }
}
