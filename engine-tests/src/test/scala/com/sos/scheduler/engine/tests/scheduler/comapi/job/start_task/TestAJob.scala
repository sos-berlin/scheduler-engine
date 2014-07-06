package com.sos.scheduler.engine.tests.scheduler.comapi.job.start_task

import sos.spooler.Job_impl

class TestAJob extends Job_impl {

  override def spooler_process() = {
    val variables = spooler.create_variable_set()
    variables.set_value("TEST", "TEST-TEST")
    spooler.job("/test-b").start(variables)
    false
  }
}
