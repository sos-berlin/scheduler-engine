package com.sos.scheduler.engine.tests.jira.js1464

/**
 * @author Joacim Zschimmer
 */
final class TestJob extends sos.spooler.Job_impl {

  override def spooler_process() = {
    Thread.sleep(spooler_task.order.params.value("sleep").toInt)
    true
  }
}
