package com.sos.scheduler.engine.plugins.jetty

import sos.spooler.Job_impl

class TestJob extends Job_impl {
  private var n = 7

  override def spooler_process = {
    spooler_log.info("Step")
    spooler_task.set_delay_spooler_process(1)
    n -= 1
    n > 0
  }
}

