package com.sos.scheduler.engine.playground.plugins.jobnet.parallel

import com.sos.scheduler.engine.data.order.OrderState
import com.sos.scheduler.engine.playground.plugins.jobnet.node.Exit
import com.sos.scheduler.engine.playground.plugins.jobnet.scheduler.Order
import com.sos.scheduler.engine.playground.plugins.jobnet.scheduler.Transaction.transaction

final case class BranchingExit(branchStarts: Set[(BranchId, OrderState)]) extends Exit {
  def orderStates = branchStarts map { _._2 }

  def moveOrder(o: Order) {
    transaction {
      for ((branchId, orderState) <- branchStarts)
        o.jobChain.addOrder(o.cloneOrder(BranchOrderId(branchId, o.id), orderState))
      o.removeFromJobChain()
    }
  }
}
