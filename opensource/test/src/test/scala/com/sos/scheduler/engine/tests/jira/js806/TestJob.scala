package com.sos.scheduler.engine.tests.jira.js806

final class TestJob extends sos.spooler.Job_impl {
  override def spooler_process() = {
    if (spooler.variables.value("TestJob.setback").toBoolean)
      spooler_task.order.setback()
    true
  }
}
