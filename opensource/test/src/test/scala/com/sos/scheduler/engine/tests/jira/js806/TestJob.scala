package com.sos.scheduler.engine.tests.jira.js806

import com.sos.scheduler.engine.common.scalautil.Logger

final class TestJob extends sos.spooler.Job_impl {
  override def spooler_process() = {
    spooler_task.order.setback()
    true
  }
}

private object TestJob {
  private val logger = Logger(getClass)
}
