package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter

import sos.spooler.Monitor_impl

final class TestSpoolerProcessBeforeMonitor extends Monitor_impl {
  override def spooler_process_before() = false
}
