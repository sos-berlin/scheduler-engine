package com.sos.scheduler.engine.tests.order.monitor.spoolertaskafter

final class TestJob extends sos.spooler.Job_impl {
  override def spooler_process() = {
    spooler_log.info("SPOOLER_PROCESS")
    spooler_task.order.params.set_value(getClass.getName, "spooler_process")
    spooler_task.end()
    true
  }
}
