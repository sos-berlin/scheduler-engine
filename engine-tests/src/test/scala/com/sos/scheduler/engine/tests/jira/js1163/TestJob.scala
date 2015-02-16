package com.sos.scheduler.engine.tests.jira.js1163

final class TestJob extends sos.spooler.Job_impl {

  override def spooler_process() = {
    Thread.sleep(9000)
    false
  }
}
