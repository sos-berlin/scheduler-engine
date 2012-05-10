package com.sos.scheduler.engine.tests.jira.js856

import sos.spooler.Job_impl

class Job1Job extends Job_impl {
  override def spooler_process() = {
    val params = spooler_task.order.params
    params.set_value("a", "a-job")
    params.set_value("b", "b-job")
    true
  }
}
