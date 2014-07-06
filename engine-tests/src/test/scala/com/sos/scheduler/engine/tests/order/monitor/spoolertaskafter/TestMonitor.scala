package com.sos.scheduler.engine.tests.order.monitor.spoolertaskafter

final class TestMonitor extends sos.spooler.Monitor_impl {

  override def spooler_task_after() {
    spooler_log.info("SPOOLER_TASK_AFTER")
    spooler_task.order.params.set_value(getClass.getName, s"exitCode=${spooler_task.exit_code}")
  }
}
