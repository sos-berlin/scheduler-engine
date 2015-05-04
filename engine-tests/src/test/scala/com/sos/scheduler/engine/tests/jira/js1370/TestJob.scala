package com.sos.scheduler.engine.tests.jira.js1370

/**
 * @author Joacim Zschimmer
 */
final class TestJob extends sos.spooler.Job_impl {

  override def spooler_process() = {
    spooler_job.set_delay_after_error(1, 12.3)
    false
  }
}
