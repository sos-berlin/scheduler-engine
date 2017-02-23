package com.sos.scheduler.engine.tests.jira.js1163

import com.sos.jobscheduler.common.time.ScalaTime._

final class TestJob extends sos.spooler.Job_impl {

  override def spooler_process() = {
    sleep(JS1163IT.UndisturbedDuration)
    false
  }
}
