package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter

import sos.spooler.Monitor_impl

final class TestMonitor extends Monitor_impl {
  override def spooler_process_after(result: Boolean) = {
    import setting.SpoolerProcessAfterNames._

    spooler_log.info("spooler_process_after("+result+")")
    val params = spooler_task.order.params
    params.set_value(parameter, result.toString)
    if (!params.value(logError).isEmpty)  spooler_log.error(params.value(logError) +" ...")
    if (!params.value(throwException).isEmpty)  throw new RuntimeException("EXCEPTION IN SPOOLER_PROCESS_AFTER")
    params.value(returns).toBoolean
  }
}