package com.sos.scheduler.engine.tests.jira.js1163

final class TestMonitor extends sos.spooler.Monitor_impl {

  override def spooler_process_before() = {
    spooler_log.info("TestMonitor")
    true
  }
}
