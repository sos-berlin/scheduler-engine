package com.sos.scheduler.engine.tests.jira.slow

import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.common.time.Stopwatch
import java.time.Instant.now

/**
  * @author Joacim Zschimmer
  */
trait ApiTimer {
  this: sos.spooler.Job_impl ⇒

  private var totalCount = 0
  private var totalDuration = 0.s

  def doVariableCalls(n: Int): Boolean = {
    val t = now
    for (_ ← 1 to n) {
      spooler_task.order.params.value("TEST")
    }
    val duration = now - t
    val callCount = 3 * n  // .order, .params, and .value("TEST")
    spooler_log.info("####### Step: " + Stopwatch.itemsPerSecondString(duration, callCount, "call"))
    totalDuration += duration
    totalCount += callCount
    true
  }

  override def spooler_close() = {
    if (totalCount > 0) {
      spooler_log.info("####### Total: " + Stopwatch.itemsPerSecondString(totalDuration, totalCount, "call"))
    }
  }
}
