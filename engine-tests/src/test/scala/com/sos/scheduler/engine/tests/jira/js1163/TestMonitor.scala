package com.sos.scheduler.engine.tests.jira.js1163

import com.sos.scheduler.engine.tests.jira.js1163.TestMonitor._

final class TestMonitor extends sos.spooler.Monitor_impl {

  override def spooler_process_before() = {
    spooler_log.info("TestMonitor")
    true
  }

  override def spooler_process_after(result: Boolean) = {
    spooler_log.info(spoolerProcessAfterString(result))
    result
  }
}

object TestMonitor {
  def spoolerProcessAfterString(result: Boolean) = s"SPOOLER_PROCESS_AFTER result=$result"
}
