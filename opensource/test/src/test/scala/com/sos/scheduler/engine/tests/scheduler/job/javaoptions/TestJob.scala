package com.sos.scheduler.engine.tests.scheduler.job.javaoptions

import sos.spooler.Job_impl

class TestJob extends Job_impl {
  override def spooler_process() = {
    spooler.variables.set_value(spooler_job.name +".myJavaOption", sys.props("myJavaOption"))
    false
  }
}
