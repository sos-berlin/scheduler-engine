package com.sos.scheduler.engine.kernel.order

import com.sos.scheduler.engine.data.order.OrderKey

final class ScalaOrderSubsystem(orderSubsystem: OrderSubsystem) {
  def orderOption(o: OrderKey) = Option(orderSubsystem.orderOrNull(o))
}

object ScalaOrderSubsystem {
  implicit def toScalaOrderSubsystem(o: OrderSubsystem) = new ScalaOrderSubsystem(o)
}
