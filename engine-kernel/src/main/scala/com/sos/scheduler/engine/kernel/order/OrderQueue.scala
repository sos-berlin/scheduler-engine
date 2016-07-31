package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.cplusplus.runtime.{Sister, SisterType}
import com.sos.scheduler.engine.data.order.OrderOverview
import com.sos.scheduler.engine.data.queries.OnlyOrderQuery
import com.sos.scheduler.engine.kernel.cppproxy.{OrderC, Order_queueC}
import scala.collection.mutable

final class OrderQueue(cppProxy: Order_queueC) extends Sister {
  def onCppProxyInvalidated() {}

  def size = cppProxy.java_order_count

  private[order] def distributedOrderOverviews(query: OnlyOrderQuery): Seq[OrderOverview] = {
    //val q = query.copy(jobChainQuery = JobChainQuery.All)
    val result = mutable.Buffer[OrderOverview]()
    cppProxy.java_for_each_distributed_order(
      limit = Integer.MAX_VALUE,
      callback = new OrderCallback {
        def apply(orderC: OrderC) = {
          val order = orderC.getSister
          if (query matchesOrder order.queryable) {
            result += orderC.getSister.overview
          }
        }
      })
    result
  }
}

object OrderQueue {

  object Type extends SisterType[OrderQueue, Order_queueC] {
    final def sister(proxy: Order_queueC, context: Sister): OrderQueue = new OrderQueue(proxy)
  }
}
