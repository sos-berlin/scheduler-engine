package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.setting

abstract class MethodNames(val name: String) {
  protected val prefix = name +"."
  val throwException = prefix +"throwException"
  val logError = prefix +"logError"
  val returns = prefix +"returns"
}

object SpoolerProcessNames extends MethodNames("SpoolerProcess")

object SpoolerProcessAfterNames extends MethodNames("SpoolerProcessAfter") {
  val parameter = prefix +"spoolerProcessAfterParameter"
}
