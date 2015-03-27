package com.sos.scheduler.engine.tests.jira.js1145

import com.sos.scheduler.engine.tests.jira.js1145.JS1145IT.mark
import com.sos.scheduler.engine.tests.jira.js1145.TestBMonitor._

/**
 * @author Joacim Zschimmer
 */
final class TestBMonitor extends sos.spooler.Monitor_impl {

  override def spooler_process_before = {
    spooler_log.info(mark(PreStepString))
    true
  }

  override def spooler_process_after(returnCode: Boolean) = {
    spooler_log.info(mark(PostStepString))
    returnCode
  }
}

private[js1145] object TestBMonitor {
  private[js1145] val PreStepString = "TestBMonitor PRE-STEP"
  private[js1145] val PostStepString = "TestBMonitor POST-STEP"
}
