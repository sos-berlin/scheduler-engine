package com.sos.scheduler.engine.tests.jira.js946

/**
 * @author Joacim Zschimmer
 */
final class TestJob extends sos.spooler.Job_impl {
  override def spooler_process() = {
    Thread.sleep(1000)
    spooler_task.end()
    true
  }
}
