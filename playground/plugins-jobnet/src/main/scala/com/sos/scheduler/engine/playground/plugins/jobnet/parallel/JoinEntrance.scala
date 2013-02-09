package com.sos.scheduler.engine.playground.plugins.jobnet.parallel

import com.sos.scheduler.engine.data.order.{OrderId, OrderState}
import com.sos.scheduler.engine.playground.plugins.jobnet.node.Entrance
import com.sos.scheduler.engine.playground.plugins.jobnet.scheduler.Transaction.transaction
import com.sos.scheduler.engine.playground.plugins.jobnet.scheduler.{JobChain, Order}

final case class JoinEntrance(state: OrderState, branchIds: Set[BranchId]) extends Entrance {
  def apply(arrivedBranchOrder: Order) {
    arrivedBranchOrder.id match {
      case BranchOrderId(branchId, originalOrderId) if branchIds contains branchId =>
        new Joiner(originalOrderId, arrivedBranchOrder).apply()
      case _ =>
    }
  }

  private class Joiner(originalOrderId: OrderId, arrivedBranchOrder: Order) {
    val jobChain = arrivedBranchOrder.jobChain
    val allBranchOrderIds = branchIds map { _.branchOrderId(originalOrderId) }

    def apply() {
      transaction {
        if (otherOrdersArrived)
          moveOrder()
        else {
          arrivedBranchOrder.moveTo(state)
          arrivedBranchOrder.companionStop()
        }
      }
    }

    def otherOrdersArrived =
      allBranchOrderIds - arrivedBranchOrder.id forall { o => jobChain.order(o).state == state }

    def moveOrder() {
      jobChain.addOrder(makeJoinedOrder(jobChain, originalOrderId, allBranchOrderIds))
      for (orderId <- allBranchOrderIds) jobChain.order(orderId).removeFromJobChain()
    }
  }

  private def makeJoinedOrder(jobChain: JobChain, orderId: OrderId, branchOrderIds: Iterable[OrderId]) = {
    val someBranchOrderId = branchOrderIds.toSeq.sortBy(_.string).head
    jobChain.order(someBranchOrderId).cloneOrder(orderId)
  }
}
