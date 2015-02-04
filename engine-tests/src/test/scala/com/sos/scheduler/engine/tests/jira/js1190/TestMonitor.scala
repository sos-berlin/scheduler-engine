package com.sos.scheduler.engine.tests.jira.js1190

import com.sos.scheduler.engine.tests.jira.js1190.TestMonitor._

/**
 * @author Joacim Zschimmer
 */
final class TestMonitor extends sos.spooler.Monitor_impl {

  override def spooler_process_before() = orderParam(BeforeProcessParam).toBoolean

  override def spooler_process_after(result: Boolean) = orderParam(AfterProcessParam).toBoolean

  private def orderParam(name: String) = spooler_task.order.params.value(name)
}

object TestMonitor {
  private[js1190] val BeforeProcessParam = "SpoolerProcessBeforeResult"
  private[js1190] val AfterProcessParam = "SpoolerProcessAfterResult"
}
