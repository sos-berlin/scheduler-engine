package com.sos.scheduler.engine.tests.scheduler.job.stdout

import TestMonitor._

/**
  * @author Joacim Zschimmer
  */
final class TestMonitor extends sos.spooler.Monitor_impl {

  System.out.println(StartStdoutMessage)
  System.err.println(StartStderrMessage)

  override def spooler_task_after() = {
    System.out.println(AfterStdoutMessage)
    System.err.println(AfterStderrMessage)
  }
}

private[stdout] object TestMonitor {
  val StartStdoutMessage = "MONITOR-START-STDOUT"
  val StartStderrMessage = "MONITOR-START-STDOUT"
  val AfterStdoutMessage = "MONITOR-AFTER-STDOUT"
  val AfterStderrMessage = "MONITOR-AFTER-STDOUT"
}
