package com.sos.scheduler.engine.tests.jira.js973

final class TestJob extends sos.spooler.Job_impl {
  override def spooler_process() = {
    val env = System.getenv("TEST_WHICH_SCHEDULER")
    spooler_task.order.params.set_value("result", s"*$env* taskId=${spooler_task.id}*")
    true
  }
}
