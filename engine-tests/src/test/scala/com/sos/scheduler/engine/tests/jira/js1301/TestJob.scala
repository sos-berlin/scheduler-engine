package com.sos.scheduler.engine.tests.jira.js1301

import java.lang.Thread.sleep

/**
 * @author Joacim Zschimmer
 */
final class TestJob extends sos.spooler.Job_impl {

  override def spooler_process() = {
    spooler_log.info(s"API TEST_AGENT=/${sys.env("TEST_AGENT")}/")
    spooler_task.order.params.value("sleep") match {
      case "" ⇒
      case seconds ⇒ sleep(seconds.toInt * 1000)
    }
    true
  }
}
