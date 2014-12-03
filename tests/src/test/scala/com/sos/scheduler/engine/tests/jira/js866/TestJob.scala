package com.sos.scheduler.engine.tests.jira.js866

class TestJob extends sos.spooler.Job_impl {
  override def spooler_process() = {
    Thread.sleep(15*1000)
    false
  }
}
