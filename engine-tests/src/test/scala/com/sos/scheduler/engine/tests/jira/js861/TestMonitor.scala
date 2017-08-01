package com.sos.scheduler.engine.tests.jira.js861

import com.sos.scheduler.engine.tests.jira.js861.TestJob.whoami

/**
  * @author Joacim Zschimmer
  */
final class TestMonitor extends sos.spooler.Monitor_impl {

  override def spooler_process_before() = {
    spooler_log.info(s"TEST-USERNAME=SELF-TEST")
    spooler_log.info(s"TEST-USERNAME=$whoami")
    true
  }
}
