package com.sos.scheduler.engine.tests.jira.js856

import sos.spooler.Job_impl

class TestJob extends Job_impl {
  override def spooler_process() = {
    val order = spooler_task.order
    val params = order.params
    params.set_value("a", "a-job")
    params.set_value("b", "b-job")
    true
  }
}
