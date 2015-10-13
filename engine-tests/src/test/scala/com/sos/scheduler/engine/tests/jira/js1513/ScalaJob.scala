package com.sos.scheduler.engine.tests.jira.js1513

/**
 * @author Joacim Zschimmer
 */
final class ScalaJob extends sos.spooler.Job_impl {

  override def spooler_process() = {
    spooler_log.info("Scala history_id=" + spooler_task.order.history_id)
    true
  }
}
