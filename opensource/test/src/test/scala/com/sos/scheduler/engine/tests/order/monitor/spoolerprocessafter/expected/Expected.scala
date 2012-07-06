package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.expected

case class Expected(orderStateExpectation: OrderStateExpectation, details: ExpectedDetail*) {
  override def toString = (orderStateExpectation :: details.toList) mkString ","
}

